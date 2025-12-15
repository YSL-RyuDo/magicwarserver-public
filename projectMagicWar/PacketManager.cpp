#include "PacketManager.h"

#include "IOCP_Send.h"
#include "UserManager.h"
#include "PasswordHasher.h"
#include "MatchManager.h"
#include "PacketStruct.h"

#include <iostream>
#include <random>
#include <vector>
#include <cstring>
#include <algorithm>
#include <sstream>

// ---------------------------
// static members
// ---------------------------
std::unordered_map<std::string, Session*>* PacketManager::s_userMap = nullptr;
MatchManager* PacketManager::s_matchMgr = nullptr;
IOCP_Send* PacketManager::s_sendProc = nullptr;
std::mutex PacketManager::s_userMapMtx;
// ---------------------------
// util
// ---------------------------
static uint32_t GenerateRandomToken()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dis;
	return dis(gen);
}

// ---------------------------
// Init
// ---------------------------
void PacketManager::Init(
	std::unordered_map<std::string, Session*>* userMap,
	MatchManager* matchMgr,
	IOCP_Send* sendProc)
{
	s_userMap = userMap;
	s_matchMgr = matchMgr;
	s_sendProc = sendProc;
}

// ---------------------------
// Packet Entry
// ---------------------------
void PacketManager::HandlePacket(
	Session* session,
	int packetId,
	const char* body,
	int bodySize)
{	
	if (!session || session->closing)
		return;

	// ---------------------------
	// [SECURITY CHECK]
	// ---------------------------
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
	case PKT_PING:
		HandlePING(session);
		break;
	default:
		std::cout << "[WARN] Unknown packetId=" << packetId << "\n";
		break;
	}
}

void PacketManager::HandlePING(Session* s)
{
	if (!s) return;

	std::cout << "[HELLO] from sessionId=" << s->sessionId << "\n";

	// 서버 응답
	SendPacket(
		s,
		PKT_PING_ACK,
		nullptr,
		0
	);
}


// ---------------------------
// Send Packet
// ---------------------------
void PacketManager::SendPacket(
	Session* session,
	uint16_t packetId,
	const void* data,
	uint16_t dataSize)
{
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

void PacketManager::HandleSessionDisconnect(Session* s)
{
	if (!s) return;

	if (!s->userId.empty())
	{
		UserManager::LogoutUser(s->userId);

		std::lock_guard<std::mutex> lock(s_userMapMtx);
		s_userMap->erase(s->userId);
	}

	s->closing = true;
}

// Signup
void PacketManager::HandleSignup(Session* s, const char* body, int size)
{
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


// Login
void PacketManager::HandleLogin(Session* s, const char* body, int size)
{
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

	// 🔑 여기서 비밀번호 검증은 이미 끝남
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

	// ===== 로그인 성공 처리 =====
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

// Profile
void PacketManager::HandleProfileRequest(Session* s, const char* body, int bodySize)
{
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

	// ------------------------
	// 유저 ID 추출
	// ------------------------
	char idBuf[33]{};
	memcpy(idBuf, body, 32);
	std::string userId(idBuf);

	std::cout << "[PROFILE] 요청 userId=" << userId << "\n";

	// ------------------------
	// 프로필 로드 (DB)
	// ------------------------
	UserProfileData userProfile{};
	if (!UserManager::LoadUserProfile(userId, userProfile))
	{
		std::cout << "[PROFILE] 프로필 로드 실패\n";
		return;
	}

	const UserInfoData& info = userProfile.info;
	const UserRankData& rankData = userProfile.rank;

	// ------------------------
	// 랭크 테이블 조회
	// ------------------------
	const RankInfo* rankInfo = UserManager::GetRankInfo(rankData.rank);
	if (!rankInfo)
	{
		std::cout << "[PROFILE] RankInfo 조회 실패 rank="
			<< rankData.rank << "\n";
		return;
	}

	// ------------------------
	// 프로필 응답 패킷 구성
	// ------------------------
	PKT_RES_PROFILE_DATA pkt{};

	// nickname은 로그인 시 세션에 저장되어 있어야 정상 구조
	memcpy(pkt.nickname,
		s->nickname.c_str(),
		std::min<size_t>(31, s->nickname.size()));

	pkt.level = info.level;
	pkt.exp = info.exp;
	pkt.rank = rankData.rank;
	pkt.rankScore = rankData.rankScore;

	memcpy(pkt.rankName,
		rankInfo->name.c_str(),
		std::min<size_t>(15, rankInfo->name.size()));

	pkt.minScore = rankInfo->minScore;
	pkt.maxScore = rankInfo->maxScore;

	// ------------------------
	// 패킷 전송
	// ------------------------
	std::cout << "[PROFILE] 프로필 패킷 전송\n";
	SendPacket(s, PKT_RES_PROFILE, &pkt, sizeof(pkt));
}

void PacketManager::HandleLogout(Session* s)
{
	if (!s || s->userId.empty())
		return;

	std::cout << "[LOGOUT] user=" << s->userId << "\n";

	if (s_matchMgr)
		s_matchMgr->CancelMatch(s);

	UserManager::LogoutUser(s->userId);

	s->closing = true;
}



void PacketManager::HandleLobbyChat(Session* sender, const char* body, int bodySize)
{
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

		//  여기서 “누구에게 보내는지” 확인 가능
		std::cout << "  → 수신자: "
			<< session->userId
			<< " (token=" << session->token << ")"
			<< std::endl;

		SendPacket(session, PKT_CHAT_LOBBY, &pkt, sizeof(pkt));
	}

	std::cout << "[LOBBY CHAT] 브로드캐스트 완료\n";
}

void PacketManager::HandleUserMagicRequest(Session* s)
{
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

void PacketManager::HandleUserMagicDetail(Session* s, const char* body, int bodySize)
{
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

	strncpy_s(
		pkt.name,
		sizeof(pkt.name),
		card->name.c_str(),
		_TRUNCATE
	);

	strncpy_s(
		pkt.effectType,
		sizeof(pkt.effectType),
		card->effectType.c_str(),
		_TRUNCATE
	);


	SendPacket(s, PKT_RES_MAGIC_DETAIL, &pkt, sizeof(pkt));

	std::cout << "[MAGIC DETAIL] 전송 완료 id=" << req.cardId << "\n";
}

void PacketManager::HandleReplaceUserMagic(Session* s, const char* body, int bodySize)
{
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

	// 서버 상태 갱신
	s->posX = pkt.x;
	s->posY = pkt.y;

	cout << "[MOVE] 이동중 user=" << s->userId
		<< " x=" << pkt.x
		<< " y=" << pkt.y << endl;

	// 핵심: 상대에게 이동 패킷 전달
	PacketManager::SendPacket(
		s->battleOpponent,
		PKT_MOVE,
		&pkt,
		sizeof(pkt)
	);
}

void PacketManager::HandleHit(Session* attacker, const char* body, int bodySize)
{
	// ─────────────────────────────
	// 0. 패킷 크기 검증
	// ─────────────────────────────
	if (bodySize != sizeof(PKT_HIT_STRUCT))
		return;

	PKT_HIT_STRUCT pkt{};
	memcpy(&pkt, body, sizeof(pkt));

	// ─────────────────────────────
	// 1. 공격자 검증
	// ─────────────────────────────
	if (!attacker)
		return;

	if (attacker->userId != pkt.attackerId)
		return;

	// ─────────────────────────────
	// 2. 타겟 세션 찾기
	// ─────────────────────────────
	Session* target = nullptr;
	{
		std::lock_guard<std::mutex> lock(s_userMapMtx);
		auto it = s_userMap->find(pkt.targetId);
		if (it != s_userMap->end())
			target = it->second;
	}

	if (!target)
		return;

	// 자기 자신 공격 방지
	if (attacker->userId == target->userId)
		return;

	// ─────────────────────────────
	// 3. 전투 상태 검증
	// ─────────────────────────────
	if (!attacker->inBattle || !target->inBattle)
		return;

	if (attacker->roomId != target->roomId)
		return;

	// ─────────────────────────────
	// 4. 데미지 처리
	// ─────────────────────────────
	int dmg = pkt.damage;
	if (dmg < 1)
		dmg = 1;
	else if (dmg > 1000)
		dmg = 1000;

	target->currentHp -= dmg;
	if (target->currentHp < 0)
		target->currentHp = 0;

	std::cout
		<< "[HIT] "
		<< attacker->userId << " -> "
		<< target->userId
		<< " card=" << pkt.cardId
		<< " dmg=" << dmg
		<< " hp=" << target->currentHp
		<< std::endl;

	// ─────────────────────────────
	// 5. 카드 ID 기반 상태이상 판정
	// ─────────────────────────────
	constexpr int CARD_ICEBALL = 1; // 슬로우
	constexpr int CARD_ELECTRICBALL = 2; // 스턴

	uint64_t now = GetTickCount64();

	// 카드 1 : 아이스볼 → 슬로우
	if (pkt.cardId == CARD_ICEBALL)
	{
		float slowRatio = 0.5f;   // 이동속도 50%
		int durationMs = 3000;   // 3초

		target->isSlowed = true;
		target->slowRatio = slowRatio;
		target->slowEndTime = now + durationMs;

		SendSlow(target, slowRatio, durationMs);

		std::cout
			<< "[SLOW] applied to "
			<< target->userId
			<< " ratio=" << slowRatio
			<< " duration=" << durationMs
			<< std::endl;
	}
	// 카드 2 : 일렉트릭볼 → 스턴
	else if (pkt.cardId == CARD_ELECTRICBALL)
	{
		int durationMs = 5000;

		target->isStunned = true;
		target->stunEndTime = now + durationMs;

		SendStun(target, durationMs);

		std::cout
			<< "[STUN] applied to "
			<< target->userId
			<< " duration=" << durationMs
			<< std::endl;
	}

	// ─────────────────────────────
	// 6. HP 결과 전송
	// ─────────────────────────────
	SendHPResult(attacker);
	SendHPResult(target);

	// ─────────────────────────────
	// 7. 사망 판정
	// ─────────────────────────────
	if (target->currentHp <= 0)
	{
		HandleDie(attacker, target);
	}
}


void PacketManager::HandleDie(Session* winner, Session* loser)
{
	if (!winner || !loser)
		return;

	std::cout << "[DIE] "
		<< loser->userId
		<< " defeated by "
		<< winner->userId
		<< std::endl;

	// -----------------------------
	// 1. DIE 패킷 즉시 전송
	// -----------------------------
	PKT_DIE_STRUCT diePkt{};
	strncpy_s(diePkt.deadUserId,
		sizeof(diePkt.deadUserId),
		loser->userId.c_str(),
		_TRUNCATE);

	SendPacket(winner, PKT_DIE, &diePkt, sizeof(diePkt));
	SendPacket(loser, PKT_DIE, &diePkt, sizeof(diePkt));

	// -----------------------------
	// 2. 지연 결과 처리 (this 제거)
	// -----------------------------
	std::thread([winner, loser]()
		{
			std::this_thread::sleep_for(std::chrono::seconds(3));

			PKT_RESULT_STRUCT victory{};
			strncpy_s(victory.userId,
				sizeof(victory.userId),
				winner->userId.c_str(),
				_TRUNCATE);

			PKT_RESULT_STRUCT defeat{};
			strncpy_s(defeat.userId,
				sizeof(defeat.userId),
				loser->userId.c_str(),
				_TRUNCATE);

			SendPacket(winner, PKT_VICTORY, &victory, sizeof(victory));
			SendPacket(loser, PKT_DEFEAT, &defeat, sizeof(defeat));

			MatchManager::ProcessResult(winner, loser);

			winner->inBattle = false;
			loser->inBattle = false;
			winner->battleOpponent = nullptr;
			loser->battleOpponent = nullptr;

		}).detach();
}



void PacketManager::HandleCastMagic(Session* s, const char* body, int bodySize)
{
	// 1. 기본 검증
	if (!s || bodySize != sizeof(PKT_CAST_MAGIC_STRUCT))
		return;

	if (!s->inBattle || !s->battleOpponent)
		return;

	// 2. CAST 패킷 파싱 (방향 벡터)
	PKT_CAST_MAGIC_STRUCT recv{};
	memcpy(&recv, body, sizeof(recv));

	// 3. 카드 정보 조회 (UserManager 기준)
	const CardInfo* card = UserManager::GetCardInfo(recv.cardId);
	if (!card)
	{
		std::cout << "[CAST_MAGIC] CardInfo not found id="
			<< recv.cardId << std::endl;
		return;
	}

	// 4. SPAWN 패킷 구성
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

	// 5. 상대 클라이언트에게만 전송
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

void PacketManager::HandleMagicDetailGAME(
	Session* s,
	const char* body,
	int bodySize)
{
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

	// 카드 정보 확인용 로그
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



// Ack helpers
void PacketManager::SendSignupAck(Session* s, uint16_t result)
{
	PKT_SIGNUP_ACK ack{};
	ack.result = result;
	SendPacket(s, result, &ack, sizeof(ack));
}

void PacketManager::SendLoginAck(Session* s, uint16_t result)
{
	PKT_LOGIN_ACK_STRUCT ack{};
	ack.result = result;
	SendPacket(s, result, &ack, sizeof(ack));
}

void PacketManager::SendHPResult(Session* s)
{
	if (!s) return;

	PKT_RES_HP_STRUCT pkt{};
	pkt.currentHp = s->currentHp;
	pkt.maxHp = s->maxHp;

	SendPacket(
		s,
		PKT_RES_HP,
		&pkt,
		sizeof(pkt)
	);
}

void PacketManager::SendSlow(Session* target, float ratio, int durationMs)
{
	PKT_STATUS_SLOW_STRUCT pkt{};
	strcpy_s(pkt.targetId, target->userId.c_str());
	pkt.slowRatio = ratio;
	pkt.durationMs = durationMs;

	SendPacket(target, PKT_STATUS_SLOW, &pkt, sizeof(pkt));
}

void PacketManager::SendStun(Session* target, int durationMs)
{
	PKT_STATUS_STUN_STRUCT pkt{};
	strcpy_s(pkt.targetId, target->userId.c_str());
	pkt.durationMs = durationMs;

	SendPacket(target, PKT_STATUS_STUN, &pkt, sizeof(pkt));
}
