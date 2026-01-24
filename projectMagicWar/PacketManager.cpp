#include "PacketManager.h"

#include "IOCP_Send.h"
#include "UserManager.h"
#include "PasswordHasher.h"
#include "MatchManager.h"
#include "PacketStruct.h"
#include "DBWorkerInstance.h"

#include <iostream>
#include <random>
#include <vector>
#include <cstring>
#include <algorithm>
#include <sstream>

std::unordered_map<std::string, Session*>* PacketManager::s_userMap = nullptr;
MatchManager* PacketManager::s_matchMgr = nullptr;
IOCP_Send* PacketManager::s_sendProc = nullptr;
std::mutex PacketManager::s_userMapMtx;

// 로그인 성공 시 세션에서 사용할 랜덤 토큰을 생성한다.
static uint32_t GenerateRandomToken()
{
	std::cout << "[TOKEN] GenerateRandomToken 시작\n";

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dis;
	return dis(gen);
}

// PacketManager가 사용할 전역 객체 포인터들을 초기화한다.
void PacketManager::Init(
	std::unordered_map<std::string, Session*>* userMap,
	MatchManager* matchMgr,
	IOCP_Send* sendProc)
{
	std::cout << "[INIT] PacketManager 초기화 시작\n";

	s_userMap = userMap;
	s_matchMgr = matchMgr;
	s_sendProc = sendProc;
}

// 모든 네트워크 패킷의 진입 지점에서 보안 검증 후 처리 함수로 분기한다.
void PacketManager::HandlePacket(
	Session* session,
	int packetId,
	uint32_t recvToken,
	const char* body,
	int bodySize)
{
	std::cout << "[PACKET] HandlePacket 시작\n";

	if (!session || session->closing)
		return;

	bool authFree =
		packetId == PKT_LOGIN ||
		packetId == PKT_SIGNUP ||
		packetId == PKT_LOGOUT ||
		packetId == PKT_PING ||
		packetId == PKT_PING_ACK;

	if (!authFree)
	{
		if (!session->authenticated)
		{
			std::cout << "[SECURITY] unauthenticated packet "
				<< "session=" << session->sessionId
				<< " packetId=" << packetId
				<< "\n";
			session->closing = true;
			return;
		}

		if (session->token == 0)
		{
			std::cout << "[SECURITY] token missing "
				<< "session=" << session->sessionId
				<< "\n";
			session->closing = true;
			return;
		}

		if (recvToken != session->token)
		{
			std::cout << "[SECURITY] token mismatch "
				<< "recv=" << recvToken
				<< " expected=" << session->token
				<< "\n";
			session->closing = true;
			return;
		}
	}

	switch (packetId)
	{
	case PKT_SIGNUP:
		HandleSignup(session, body, bodySize);
		break;

	case PKT_LOGIN:
		HandleLogin(session, body, bodySize);
		break;
	case PKT_LOGOUT:
		HandleLogout(session);
		break;
	case PKT_REQ_PROFILE:
		HandleProfileRequest(session, body, bodySize);
		break;
	case PKT_CHAT_LOBBY:
		HandleLobbyChat(session, body, bodySize);
		break;
	case PKT_REQ_USER_MAGIC:
		HandleUserMagicRequest(session);
		break;
	case PKT_REQ_MAGIC_DETAIL:
		HandleUserMagicDetail(session, body, bodySize);
		break;
	case PKT_REQ_REPLACE_USER_MAGIC:
		HandleReplaceUserMagic(session, body, bodySize);
		break;
	case PKT_MATCH_REQUEST:
		s_matchMgr->RequestMatch(session);
		break;
	case PKT_MATCH_CANCEL:
		s_matchMgr->CancelMatch(session);
		break;
	case PKT_MOVE:
		HandleMove(session, body, bodySize);
		break;
	case PKT_REQ_HP:
	{
		PKT_RES_HP_STRUCT pkt{};
		strncpy_s(pkt.userId, sizeof(pkt.userId),
			session->userId.c_str(), _TRUNCATE);

		pkt.currentHp = session->currentHp;
		pkt.maxHp = session->maxHp;

		SendPacket(
			session,
			PKT_RES_HP,
			&pkt,
			sizeof(pkt)
		);
		break;
	}
	case PKT_HIT:
		HandleHit(session, body, bodySize);
		break;
	case PKT_CAST_MAGIC:
		HandleCastMagic(session, body, bodySize);
		break;
	case PKT_REQ_MAGIC_DETAIL_GAME:
		HandleMagicDetailGAME(session, body, bodySize);
		break;
	case PKT_CAST_FIELD_MAGIC:
		HandleCastFieldMagic(session, body, bodySize);
		break;
	case PKT_PING:
		HandlePING(session);
		break;
	default:
		std::cout << "[WARN] Unknown packetId=" << packetId << "\n";
		break;
	}
}

// 클라이언트의 Ping 요청에 대해 응답 패킷을 전송한다.
void PacketManager::HandlePING(Session* s)
{
	std::cout << "[PING] HandlePING 시작\n";

	if (!s) return;

	std::cout << "[HELLO] from sessionId=" << s->sessionId << "\n";

	SendPacket(
		s,
		PKT_PING_ACK,
		nullptr,
		0
	);
}

// 패킷 헤더를 포함한 데이터를 IOCP 송신 큐에 등록한다.
void PacketManager::SendPacket(
	Session* session,
	uint16_t packetId,
	const void* data,
	uint16_t dataSize)
{
	std::cout << "[SEND] SendPacket 시작\n";

	if (!session || session->closing)
		return;

	PACKET_HEADER header{};
	header.packetId = packetId;
	header.token = session->token;
	header.bodySize = dataSize;

	std::vector<char> buf(sizeof(PACKET_HEADER) + dataSize);
	memcpy(buf.data(), &header, sizeof(header));
	memcpy(buf.data() + sizeof(header), data, dataSize);

	s_sendProc->EnqueueSend(session, buf.data(), (int)buf.size());
}

// 세션 종료 시 유저 정보를 정리하고 유저 맵에서 제거한다.
void PacketManager::HandleSessionDisconnect(Session* s)
{
	std::cout << "[DISCONNECT] HandleSessionDisconnect 시작\n";

	if (!s) return;

	if (!s->userId.empty())
	{
		UserManager::LogoutUser(s->userId);

		std::lock_guard<std::mutex> lock(s_userMapMtx);
		s_userMap->erase(s->userId);
	}

	s->closing = true;
}

// 회원가입 요청을 처리하고 신규 계정을 생성한다.
void PacketManager::HandleSignup(Session* s, const char* body, int size)
{
	std::cout << "[SIGNUP] 처리 시작\n";

	std::cout << "[SIGNUP] HandleSignup 진입 size=" << size << "\n";

	if (size != sizeof(PKT_SIGNUP_REQ))
	{
		std::cout << "[SIGNUP] size 불일치\n";
		return;
	}

	PKT_SIGNUP_REQ req{};
	memcpy(&req, body, sizeof(req));

	std::string id(req.id);
	std::string pw(req.pw);
	std::string nick(req.nick);

	std::cout << "[SIGNUP] 요청 id=" << id << " nick=" << nick << "\n";

	if (UserManager::IsIDExist(id))
	{
		std::cout << "[SIGNUP] ID 이미 존재\n";
		SendSignupAck(s, PKT_SIGNUP_ID_EXISTS);
		return;
	}

	if (UserManager::IsNickExist(nick))
	{
		std::cout << "[SIGNUP] Nick 이미 존재\n";
		SendSignupAck(s, PKT_SIGNUP_NICK_EXISTS);
		return;
	}

	std::string salt = PasswordHasher::GenerateSalt(16);
	std::string hash = PasswordHasher::Hash(pw, salt);

	UserManager::AddUser(id, hash, salt, nick);

	std::cout << "[SIGNUP] 성공\n";
	SendSignupAck(s, PKT_SIGNUP_OK);
}


// 로그인 요청을 처리하고 세션 인증 상태를 설정한다.
void PacketManager::HandleLogin(Session* s, const char* body, int size)
{
	std::cout << "[LOGIN] 처리 시작\n";

	std::cout << "[LOGIN] HandleLogin 진입 size=" << size << "\n";

	if (size != sizeof(PKT_LOGIN_REQ))
	{
		std::cout << "[LOGIN] size 불일치\n";
		return;
	}

	PKT_LOGIN_REQ req{};
	memcpy(&req, body, sizeof(req));

	std::string id(req.id);
	std::string pw(req.pw);
	std::string nick;

	std::cout << "[LOGIN] 로그인 시도 id=" << id << "\n";

	if (!UserManager::CheckLogin(id, pw, nick))
	{
		std::cout << "[LOGIN] 비밀번호 불일치 또는 ID 없음\n";
		SendLoginAck(s, PKT_LOGIN_PW_FAIL);
		return;
	}

	if (!UserManager::TryLoginUser(id))
	{
		std::cout << "[LOGIN] 이미 로그인 상태\n";
		SendLoginAck(s, PKT_LOGIN_ALREADY);
		return;
	}

	s->userId = id;
	s->nickname = nick;
	s->authenticated = true;
	s->token = GenerateRandomToken();

	{
		std::lock_guard<std::mutex> lock(s_userMapMtx);
		(*s_userMap)[id] = s;
	}

	PKT_LOGIN_ACK_STRUCT ack{};
	ack.result = PKT_LOGIN_OK;
	ack.token = s->token;
	strncpy_s(ack.nick, nick.c_str(), 31);

	std::cout << "[LOGIN] ACK 전송 token=" << s->token << "\n";
	SendPacket(s, PKT_LOGIN_OK, &ack, sizeof(ack));

	std::cout << "[LOGIN] 완료 user=" << id << "\n";
}


// 유저 프로필 정보를 조회하여 클라이언트로 전달한다.
void PacketManager::HandleProfileRequest(Session* s, const char* body, int bodySize)
{
	std::cout << "[PROFILE] 처리 시작\n";

	std::cout << "[PROFILE] 요청 수신 bodySize=" << bodySize << "\n";

	if (!s)
	{
		std::cout << "[PROFILE] 세션 없음\n";
		return;
	}

	if (bodySize != 32)
	{
		std::cout << "[PROFILE] bodySize 오류\n";
		return;
	}

	char idBuf[33]{};
	memcpy(idBuf, body, 32);
	std::string userId(idBuf);

	std::cout << "[PROFILE] 요청 userId=" << userId << "\n";

	UserProfileData userProfile{};
	if (!UserManager::LoadUserProfile(userId, userProfile))
	{
		std::cout << "[PROFILE] 프로필 로드 실패\n";
		return;
	}

	const UserInfoData& info = userProfile.info;
	const UserRankData& rankData = userProfile.rank;

	const RankInfo* rankInfo = UserManager::GetRankInfo(rankData.rank);
	if (!rankInfo)
	{
		std::cout << "[PROFILE] RankInfo 조회 실패 rank="
			<< rankData.rank << "\n";
		return;
	}

	PKT_RES_PROFILE_DATA pkt{};

	memcpy(
		pkt.nickname,
		s->nickname.c_str(),
		std::min<size_t>(31, s->nickname.size())
	);

	pkt.level = info.level;
	pkt.exp = info.exp;
	pkt.rank = rankData.rank;
	pkt.rankScore = rankData.rankScore;

	memcpy(
		pkt.rankName,
		rankInfo->name.c_str(),
		std::min<size_t>(15, rankInfo->name.size())
	);

	pkt.minScore = rankInfo->minScore;
	pkt.maxScore = rankInfo->maxScore;

	std::cout << "[PROFILE] 프로필 패킷 전송\n";
	SendPacket(s, PKT_RES_PROFILE, &pkt, sizeof(pkt));
}


// 로그아웃 요청을 처리하고 세션을 종료 상태로 전환한다.
void PacketManager::HandleLogout(Session* s)
{
	std::cout << "[LOGOUT] 처리 시작\n";

	if (!s || s->userId.empty())
		return;

	std::cout << "[LOGOUT] user=" << s->userId << "\n";

	if (s_matchMgr)
		s_matchMgr->CancelMatch(s);

	UserManager::LogoutUser(s->userId);

	s->closing = true;
}


// 로비 채팅 메시지를 접속 중인 모든 유저에게 브로드캐스트한다.
void PacketManager::HandleLobbyChat(Session* sender, const char* body, int bodySize)
{
	std::cout << "[LOBBY CHAT] 처리 시작\n";

	if (bodySize <= 0 || sender->inBattle)
		return;

	PKT_CHAT_LOBBY_DATA pkt{};
	strncpy_s(pkt.nickname, sender->nickname.c_str(), 31);
	memcpy(pkt.message, body, min(bodySize, 127));

	std::lock_guard<std::mutex> lock(s_userMapMtx);

	std::cout << "[LOBBY CHAT] 송신자 = "
		<< sender->userId
		<< " / 메시지 = "
		<< pkt.message
		<< std::endl;

	for (auto it = s_userMap->begin(); it != s_userMap->end(); ++it)
	{
		Session* session = it->second;

		if (!session || session->closing)
			continue;

		if (session->inBattle)
			continue;

		std::cout << "  → 수신자: "
			<< session->userId
			<< " (token=" << session->token << ")"
			<< std::endl;

		SendPacket(session, PKT_CHAT_LOBBY, &pkt, sizeof(pkt));
	}

	std::cout << "[LOBBY CHAT] 브로드캐스트 완료\n";
}


// 유저가 보유한 마법 슬롯 정보를 조회하여 전송한다.
void PacketManager::HandleUserMagicRequest(Session* s)
{
	std::cout << "[MAGIC] HandleUserMagicRequest 시작\n";

	if (!s || !s->authenticated)
		return;

	std::vector<int> slots;
	if (!UserManager::LoadUserMagic(s->userId, slots))
	{
		std::cout << "[MAGIC] 슬롯 로드 실패 user=" << s->userId << "\n";
		return;
	}

	PKT_RES_USER_MAGIC_STRUCT pkt{};
	pkt.slotCount = static_cast<int32_t>(slots.size());

	std::cout << "[MAGIC SLOT] user=" << s->userId << std::endl;

	for (int i = 0; i < pkt.slotCount && i < 4; i++)
	{
		pkt.cardIds[i] = slots[i];

		std::cout
			<< "  slot " << i
			<< " : cardId=" << slots[i]
			<< std::endl;
	}

	SendPacket(s, PKT_RES_USER_MAGIC, &pkt, sizeof(pkt));

	std::cout << "[MAGIC] 슬롯 바이너리 전송 완료 user=" << s->userId << "\n";
}


// 특정 마법 카드의 상세 정보를 조회하여 전송한다.
void PacketManager::HandleUserMagicDetail(Session* s, const char* body, int bodySize)
{
	std::cout << "[MAGIC DETAIL] HandleUserMagicDetail 시작\n";

	if (bodySize != sizeof(PKT_REQ_MAGIC_DETAIL_STRUCT))
	{
		std::cout << "[MAGIC DETAIL] bodySize 오류\n";
		return;
	}

	PKT_REQ_MAGIC_DETAIL_STRUCT req{};
	memcpy(&req, body, sizeof(req));

	const CardInfo* card = UserManager::GetCardInfo(req.cardId);
	if (!card)
	{
		std::cout << "[MAGIC DETAIL] 카드 없음 id=" << req.cardId << "\n";
		return;
	}

	PKT_RES_MAGIC_DETAIL_STRUCT pkt{};
	pkt.id = card->id;
	pkt.attack = card->attack;
	pkt.cooldown = card->cooldown;
	pkt.cost = card->cost;

	pkt.speed = card->speed;
	pkt.radius = card->radius;
	pkt.duration = card->duration;
	pkt.amount = card->amount;

	strncpy_s(pkt.name, sizeof(pkt.name), card->name.c_str(), _TRUNCATE);
	strncpy_s(pkt.effectType, sizeof(pkt.effectType), card->effectType.c_str(), _TRUNCATE);

	SendPacket(s, PKT_RES_MAGIC_DETAIL, &pkt, sizeof(pkt));

	std::cout << "[MAGIC DETAIL] 전송 완료 id=" << req.cardId << "\n";
}

// 유저의 마법 슬롯 구성을 교체하고 결과를 응답한다.
void PacketManager::HandleReplaceUserMagic(Session* s, const char* body, int bodySize)
{
	std::cout << "[MAGIC REPLACE] 처리 시작\n";

	if (!s || !s->authenticated)
		return;

	if (bodySize != sizeof(PKT_REQ_REPLACE_MAGIC_STURCT))
	{
		std::cout << "[MAGIC REPLACE] bodySize 오류\n";
		return;
	}

	PKT_REQ_REPLACE_MAGIC_STURCT req{};
	memcpy(&req, body, sizeof(req));

	bool ok = UserManager::ReplaceUserMagic(
		s->userId,
		req.oldCardId,
		req.newCardId
	);

	PKT_RES_REPLACE_USER_MAGIC_STURCT res{};
	res.result = ok ? 1 : 0;

	SendPacket(
		s,
		PKT_RES_REPLACE_USER_MAGIC,
		&res,
		sizeof(res)
	);

	std::cout << "[MAGIC REPLACE] "
		<< (ok ? "성공" : "실패")
		<< " user=" << s->userId
		<< " old=" << req.oldCardId
		<< " new=" << req.newCardId
		<< "\n";
}


// 전투 중 캐릭터의 이동 정보를 서버에 반영하고 상대에게 전달한다.
void PacketManager::HandleMove(Session* s, const char* body, int size)
{
	if (!s)
	{
		cout << "[MOVE] session nullptr\n";
		return;
	}

	if (!s->inBattle)
	{
		cout << "[MOVE] not in battle user=" << s->userId << endl;
		return;
	}

	if (!s->battleOpponent)
	{
		cout << "[MOVE] opponent null user=" << s->userId << endl;
		return;
	}

	if (size != sizeof(PKT_MOVE_STRUCT))
	{
		cout << "[MOVE] size mismatch\n";
		return;
	}

	PKT_MOVE_STRUCT pkt{};
	memcpy(&pkt, body, sizeof(pkt));

	s->posX = pkt.x;
	s->posY = pkt.y;

	PacketManager::SendPacket(
		s->battleOpponent,
		PKT_MOVE,
		&pkt,
		sizeof(pkt)
	);
}

// 공격 패킷을 처리하여 데미지와 상태 이상을 적용한다.
void PacketManager::HandleHit(Session* attacker, const char* body, int bodySize)
{
	constexpr int CARD_ICEBALL = 1;
	constexpr int CARD_ELECTRICBALL = 2;
	constexpr int CARD_ICEFIELDS = 3;
	constexpr int CARD_POISON_MIST = 4;

	if (!attacker || bodySize != sizeof(PKT_HIT_STRUCT))
		return;

	PKT_HIT_STRUCT pkt{};
	memcpy(&pkt, body, sizeof(pkt));

	if (attacker->userId != pkt.attackerId)
		return;

	Session* target = nullptr;
	{
		std::lock_guard<std::mutex> lock(s_userMapMtx);
		auto it = s_userMap->find(pkt.targetId);
		if (it != s_userMap->end())
			target = it->second;
	}

	if (!target || attacker->userId == target->userId)
		return;

	if (!attacker->inBattle || !target->inBattle)
		return;

	if (attacker->roomId != target->roomId)
		return;

	uint64_t now = GetTickCount64();

	// DOT
	if (pkt.cardId == CARD_POISON_MIST)
	{
		if (now < target->nextDotTickTime)
			return;

		target->nextDotTickTime = now + 1000;
		target->currentHp -= 5;
		if (target->currentHp < 0) target->currentHp = 0;

		SendHPResult(target);

		if (target->currentHp <= 0)
			HandleDie(attacker, target);
		return;
	}

	// FIELD
	if (pkt.cardId == CARD_ICEFIELDS)
	{
		target->isSlowed = true;
		target->slowRatio = 0.4f;
		target->slowEndTime = now + 1000;
		SendSlow(target, 0.4f, 1000);
		return;
	}

	// 일반 데미지
	int dmg = pkt.damage;
	if (dmg < 1) dmg = 1;
	else if (dmg > 1000) dmg = 1000;

	target->currentHp -= dmg;
	if (target->currentHp < 0)
		target->currentHp = 0;

	if (pkt.cardId == CARD_ICEBALL)
	{
		target->isSlowed = true;
		target->slowRatio = 0.5f;
		target->slowEndTime = now + 3000;
		SendSlow(target, 0.5f, 3000);
	}
	else if (pkt.cardId == CARD_ELECTRICBALL)
	{
		target->isStunned = true;
		target->stunEndTime = now + 5000;
		SendStun(target, 5000);
	}

	// ⭐ 투사체 제거
	PKT_REMOVE_PROJECTILE_STRUCT rm{};
	strncpy_s(rm.casterId, sizeof(rm.casterId),
		attacker->userId.c_str(), _TRUNCATE);
	rm.cardId = pkt.cardId;

	SendPacket(target, PKT_REMOVE_PROJECTILE, &rm, sizeof(rm));

	SendHPResult(attacker);
	SendHPResult(target);

	if (target->currentHp <= 0)
		HandleDie(attacker, target);
}

// 전투 종료 시 승패 처리와 결과 전송 및 DB 갱신을 수행한다.
void PacketManager::HandleDie(Session* winner, Session* loser)
{
	std::cout << "[DIE] 처리 시작\n";

	if (!winner || !loser)
		return;

	PKT_DIE_STRUCT diePkt{};
	strncpy_s(diePkt.deadUserId, loser->userId.c_str(), _TRUNCATE);

	SendPacket(winner, PKT_DIE, &diePkt, sizeof(diePkt));
	SendPacket(loser, PKT_DIE, &diePkt, sizeof(diePkt));

	std::string winId = winner->userId;
	std::string loseId = loser->userId;

	std::thread([winId, loseId, winner, loser]()
		{
			std::this_thread::sleep_for(std::chrono::seconds(3));

			PKT_RESULT_STRUCT v{}, d{};
			strncpy_s(v.userId, winId.c_str(), _TRUNCATE);
			strncpy_s(d.userId, loseId.c_str(), _TRUNCATE);

			SendPacket(winner, PKT_VICTORY, &v, sizeof(v));
			SendPacket(loser, PKT_DEFEAT, &d, sizeof(d));

			if (g_dbWorker)
			{
				g_dbWorker->Enqueue([winId, loseId]()
					{
						MatchManager::ProcessResultById(winId, loseId);
					});
			}

			winner->inBattle = false;
			loser->inBattle = false;
			winner->battleOpponent = nullptr;
			loser->battleOpponent = nullptr;

		}).detach();
}


// 마법 시전 요청을 처리하여 상대 클라이언트에 투사체를 생성한다.
void PacketManager::HandleCastMagic(Session* s, const char* body, int bodySize)
{
	std::cout << "[CAST MAGIC] 처리 시작\n";

	if (!s || bodySize != sizeof(PKT_CAST_MAGIC_STRUCT))
		return;

	if (!s->inBattle || !s->battleOpponent)
		return;

	PKT_CAST_MAGIC_STRUCT recv{};
	memcpy(&recv, body, sizeof(recv));

	const CardInfo* card = UserManager::GetCardInfo(recv.cardId);
	if (!card)
	{
		std::cout << "[CAST_MAGIC] CardInfo not found id="
			<< recv.cardId << std::endl;
		return;
	}

	PKT_SPAWN_MAGIC_STRUCT send{};
	memset(send.casterId, 0, sizeof(send.casterId));
	strncpy_s(
		send.casterId,
		sizeof(send.casterId),
		s->userId.c_str(),
		_TRUNCATE
	);

	send.cardId = recv.cardId;
	send.dirX = recv.dirX;
	send.dirY = recv.dirY;
	send.attack = card->attack;
	send.projectileSpeed = card->speed;

	SendPacket(
		s->battleOpponent,
		PKT_SPAWN_MAGIC,
		&send,
		sizeof(send)
	);

	std::cout
		<< "[SPAWN_MAGIC] from=" << s->userId
		<< " to=" << s->battleOpponent->userId
		<< " card=" << send.cardId
		<< " dir=(" << send.dirX << "," << send.dirY << ")"
		<< " atk=" << send.attack
		<< " speed=" << send.projectileSpeed
		<< std::endl;
}

void PacketManager::HandleCastFieldMagic(
	Session* s,
	const char* body,
	int bodySize)
{
	if (!s || bodySize != sizeof(PKT_CAST_FIELD_MAGIC_STRUCT))
		return;

	if (!s->inBattle || !s->battleOpponent)
		return;

	PKT_CAST_FIELD_MAGIC_STRUCT recv{};
	memcpy(&recv, body, sizeof(recv));

	const CardInfo* card = UserManager::GetCardInfo(recv.cardId);
	if (!card)
		return;

	PKT_SPAWN_FIELD_MAGIC_STRUCT send{};
	memset(&send, 0, sizeof(send));

	strncpy_s(
		send.casterId,
		sizeof(send.casterId),
		s->userId.c_str(),
		_TRUNCATE
	);

	int fieldCardId = recv.cardId;
	if (recv.cardId == 5)          // MeteorStrike
		fieldCardId = 12;

	send.cardId = fieldCardId;
	send.localX = recv.localX;
	send.localY = recv.localY;
	send.attack = card->attack;
	send.radius = card->radius;
	send.duration = card->duration;
	send.amount = card->amount;

	//SendPacket(s, PKT_SPAWN_FIELD_MAGIC, &send, sizeof(send));
	SendPacket(s->battleOpponent, PKT_SPAWN_FIELD_MAGIC, &send, sizeof(send));
}


// 게임 중 마법 카드의 상세 정보를 조회하여 전송한다.
void PacketManager::HandleMagicDetailGAME(
	Session* s,
	const char* body,
	int bodySize)
{
	std::cout << "[MAGIC DETAIL GAME] 처리 시작\n";

	if (!s || bodySize != sizeof(PKT_REQ_MAGIC_DETAIL_STRUCT_GAME))
	{
		std::cout << "[MAGIC DETAIL][ERROR] invalid session or size "
			<< "bodySize=" << bodySize
			<< " expected=" << sizeof(PKT_REQ_MAGIC_DETAIL_STRUCT_GAME)
			<< "\n";
		return;
	}

	PKT_REQ_MAGIC_DETAIL_STRUCT_GAME req{};
	memcpy(&req, body, sizeof(req));

	std::cout << "[MAGIC DETAIL][REQ] "
		<< "user=" << s->userId
		<< " cardId=" << req.cardId
		<< "\n";

	const CardInfo* card = UserManager::GetCardInfo(req.cardId);
	if (!card)
	{
		std::cout << "[MAGIC DETAIL][ERROR] card not found "
			<< "cardId=" << req.cardId
			<< "\n";
		return;
	}

	std::cout << "[MAGIC DETAIL][CARD DATA]\n"
		<< "  id        : " << card->id << "\n"
		<< "  attack    : " << card->attack << "\n"
		<< "  speed     : " << card->speed << "\n"
		<< "  cooldown  : " << card->cooldown << "\n"
		<< "  radius    : " << card->radius << "\n"
		<< "  duration  : " << card->duration << "\n"
		<< "  amount    : " << card->amount << "\n";

	PKT_RES_MAGIC_DETAIL_STRUCT_GAME res{};
	res.cardId = card->id;
	res.attack = card->attack;
	res.speed = card->speed;
	res.cooldown = card->cooldown;
	res.radius = card->radius;
	res.duration = card->duration;
	res.amount = card->amount;

	SendPacket(
		s,
		PKT_RES_MAGIC_DETAIL,
		&res,
		sizeof(res)
	);

	std::cout << "[MAGIC DETAIL][SEND] "
		<< "to=" << s->userId
		<< " cardId=" << res.cardId
		<< " atk=" << res.attack
		<< " spd=" << res.speed
		<< " cd=" << res.cooldown
		<< " rad=" << res.radius
		<< " dur=" << res.duration
		<< " amt=" << res.amount
		<< "\n";
}


// 회원가입 결과 응답 패킷을 전송한다.
void PacketManager::SendSignupAck(Session* s, uint16_t result)
{
	std::cout << "[SIGNUP ACK] 처리 시작\n";

	PKT_SIGNUP_ACK ack{};
	ack.result = result;
	SendPacket(s, result, &ack, sizeof(ack));
}


// 로그인 결과 응답 패킷을 전송한다.
void PacketManager::SendLoginAck(Session* s, uint16_t result)
{
	std::cout << "[LOGIN ACK] 처리 시작\n";

	PKT_LOGIN_ACK_STRUCT ack{};
	ack.result = result;
	SendPacket(s, result, &ack, sizeof(ack));
}


// 현재 HP 정보를 클라이언트로 전송한다.
void PacketManager::SendHPResult(Session* s)
{
	if (!s) return;

	PKT_RES_HP_STRUCT pkt{};
	strncpy_s(pkt.userId, sizeof(pkt.userId),
		s->userId.c_str(), _TRUNCATE);

	pkt.currentHp = s->currentHp;
	pkt.maxHp = s->maxHp;

	SendPacket(
		s,
		PKT_RES_HP,
		&pkt,
		sizeof(pkt)
	);
}



// 슬로우 상태 이상을 클라이언트에 적용한다.
void PacketManager::SendSlow(Session* target, float ratio, int durationMs)
{
	std::cout << "[SLOW] 처리 시작\n";

	PKT_STATUS_SLOW_STRUCT pkt{};
	strcpy_s(pkt.targetId, target->userId.c_str());
	pkt.slowRatio = ratio;
	pkt.durationMs = durationMs;

	SendPacket(target, PKT_STATUS_SLOW, &pkt, sizeof(pkt));
}


// 스턴 상태 이상을 클라이언트에 적용한다.
void PacketManager::SendStun(Session* target, int durationMs)
{
	std::cout << "[STUN] 처리 시작\n";

	PKT_STATUS_STUN_STRUCT pkt{};
	strcpy_s(pkt.targetId, target->userId.c_str());
	pkt.durationMs = durationMs;

	SendPacket(target, PKT_STATUS_STUN, &pkt, sizeof(pkt));
}
