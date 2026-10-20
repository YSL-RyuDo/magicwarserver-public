"""Run against a fresh isolated MariaDB directory. Never uses deployment data.

python tests/integration.py --mariadb-root PATH --server PATH --runtime-dir NEW_PATH
"""
import argparse
import os
from pathlib import Path
import socket
import struct
import subprocess
import time

args = argparse.ArgumentParser()
args.add_argument("--mariadb-root", type=Path, required=True)
args.add_argument("--server", type=Path, required=True)
args.add_argument("--runtime-dir", type=Path, required=True)
a = args.parse_args()
runtime = a.runtime_dir.resolve()
runtime.mkdir(parents=True, exist_ok=False)
dbroot = a.mariadb_root.resolve()
dbbin = dbroot / "bin"
flags = subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0
processes, sockets, checks = [], [], []
logs = []
def spawn(command, **kw):
    p = subprocess.Popen(command, creationflags=flags, **kw)
    processes.append(p)
    return p
def free_port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]
dbport, port = free_port(), free_port()
def wait_port(p, proc):
    deadline = time.monotonic() + 20
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError(f"process exited early: {proc.returncode}; inspect {runtime}")
        try:
            with socket.create_connection(("127.0.0.1", p), timeout=.2): return
        except OSError: time.sleep(.05)
    raise TimeoutError(f"port {p}")
def connect():
    s = socket.create_connection(("127.0.0.1", port), timeout=4)
    sockets.append(s)
    return s
def frame(kind, body=b"", token=0):
    return struct.pack("<HIH", kind, token, len(body)) + body
def field(s, n): return s.encode().ljust(n, b"\0")
def exact(s, n):
    b = b""
    while len(b) < n:
        chunk = s.recv(n-len(b))
        if not chunk: raise EOFError("connection closed")
        b += chunk
    return b
def receive(s):
    kind, token, n = struct.unpack("<HIH", exact(s, 8))
    return kind, token, exact(s, n)
def check(name, condition):
    assert condition, name
    checks.append(name)
    print("PASS:", name, flush=True)
def login(s, user="fixture", password="synthetic-only"):
    s.sendall(frame(1, field(user,32)+field(password,64)))
    kind, token, body = receive(s)
    check("login "+user, kind == 5 and token != 0 and struct.unpack("<HI",body[:6]) == (5,token))
    return token
def closed(s):
    # A logout may already have queued a match-cancel notification.
    try:
        while s.recv(4096): pass
        return True
    except (ConnectionResetError, ConnectionAbortedError): return True

try:
    subprocess.run([str(dbbin/"mariadb-install-db.exe"), "--datadir="+str(runtime/"db"),
                    "--port="+str(dbport), "--silent"], check=True,
                   creationflags=flags, stdout=subprocess.DEVNULL)
    for name in ("database", "server"):
        logs.append(open(runtime/(name+".log"), "wb"))
    db = spawn([str(dbbin/"mariadbd.exe"), "--no-defaults",
                "--basedir="+str(dbroot), "--datadir="+str(runtime/"db"),
                "--port="+str(dbport), "--bind-address=127.0.0.1",
                "--skip-name-resolve", "--console"], stdout=logs[0], stderr=subprocess.STDOUT)
    wait_port(dbport, db)
    client = [str(dbbin/"mariadb.exe"), "--no-defaults", "--host=127.0.0.1",
              "--port="+str(dbport), "--user=root"]
    schema = Path(__file__).with_name("schema.sql").read_bytes()
    subprocess.run(client, input=schema, check=True, creationflags=flags, stdout=subprocess.DEVNULL)
    env = dict(os.environ, ARCANA_DB_HOST="127.0.0.1", ARCANA_DB_PORT=str(dbport),
               ARCANA_DB_USER="root", ARCANA_DB_PASSWORD="", ARCANA_DB_NAME="arcana_test",
               ARCANA_BIND_HOST="127.0.0.1", ARCANA_PORT=str(port))
    env["PATH"] = str(dbroot/"lib")+";"+str(dbbin)+";"+env["PATH"]
    server = spawn([str(a.server.resolve())], env=env, stdin=subprocess.PIPE,
                   stdout=logs[1], stderr=subprocess.STDOUT)
    wait_port(port, server)
    s=connect()
    signup=frame(0,field("fixture",32)+field("synthetic-only",64)+field("Fixture",32))
    for part in (signup[:3], signup[3:7], signup[7:20], signup[20:]):
        s.sendall(part); time.sleep(.02)
    check("split header and body signup", receive(s)[0] == 2)
    s.sendall(signup)
    check("duplicate ID", receive(s)[0] == 3)
    s.sendall(frame(0,field("other",32)+field("synthetic-only",64)+field("Fixture",32)))
    check("duplicate nickname", receive(s)[0] == 4)
    s.sendall(frame(1,field("fixture",32)+field("wrong",64)))
    check("wrong password", receive(s)[0] == 7)
    token=login(s)
    s.sendall(frame(10,field("fixture",32),token)+frame(13,token=token))
    profile, deck = receive(s), receive(s)
    check("queued profile", profile[0] == 11 and profile[2][:7] == b"Fixture")
    check("queued deck read", deck[0] == 14 and struct.unpack("<5i",deck[2])==(4,0,1,2,3))
    s.sendall(frame(17,struct.pack("<ii",0,4),token)+frame(13,token=token))
    changed, deck=receive(s),receive(s)
    check("replace then read preserves FIFO", changed[0]==18 and changed[2]==b"\x01"
          and struct.unpack("<5i",deck[2])==(4,4,1,2,3))
    s.sendall(frame(17,struct.pack("<ii",4,99999),token))
    check("unknown card rejected", receive(s)[2]==b"\0")
    other=connect()
    other.sendall(frame(1,field("fixture",32)+field("synthetic-only",64)))
    check("duplicate login on another session", receive(other)[0]==9)
    # Real quote-bearing account must round trip as data, not SQL.
    q=connect()
    quoted="quote'\\id"
    q.sendall(frame(0,field(quoted,32)+field("synthetic-only",64)+field("Nick'\\test",32)))
    check("quote/backslash signup bound parameters",receive(q)[0]==2)
    qt=login(q,quoted)
    q.sendall(frame(10,field(quoted,32),qt))
    check("quote/backslash profile bound parameters",receive(q)[0]==11)
    attack=connect()
    attack.sendall(frame(1,field("' OR 1=1 -- ",32)+field("synthetic-only",64)))
    check("SQL-shaped login is a literal ID",receive(attack)[0]==7)
    # A 4096-byte frame followed by other frames, split at the former overflow.
    bulk=connect()
    data=frame(254,b"x"*4088)+frame(254)*100
    bulk.sendall(data[:3000]);time.sleep(.03);bulk.sendall(data[3000:])
    check("maximum frame plus merged frames after partial receive",
          all(receive(bulk)[0]==255 for _ in range(101)))
    bad=connect();bad.sendall(struct.pack("<HIH",254,0,4089))
    check("header-inclusive oversize rejected",closed(bad))
    bad=connect();bad.sendall(frame(1,b"x"*96))
    check("unterminated fixed fields rejected",closed(bad))
    # Delay the DB using a real table lock, but leave IOCP able to serve ping/logout.
    s.sendall(frame(8));check("logout closes session",closed(s))
    locker = spawn(client+["arcana_test","--batch","--skip-column-names","--unbuffered","--execute",
        "LOCK TABLES user_account WRITE; SELECT 'locked'; SELECT SLEEP(1.5); UNLOCK TABLES;"],
        stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    check("DB lock acquired",locker.stdout.readline().strip()==b"locked")
    pending=connect()
    pending.sendall(frame(1,field("fixture",32)+field("synthetic-only",64)))
    time.sleep(.1)
    start=time.monotonic();pending.sendall(frame(254))
    check("ping remains responsive during blocked login",receive(pending)[0]==255
          and time.monotonic()-start < .8)
    pending.sendall(frame(8));check("logout while login pending",closed(pending))
    locker.wait(timeout=5)
    later=connect();login(later)
    check("cancelled login does not reserve account",True)
    locker = spawn(client+["arcana_test","--batch","--skip-column-names","--unbuffered","--execute",
        "LOCK TABLES user_info WRITE, user_magic WRITE; SELECT 'locked'; SELECT SLEEP(1.5); UNLOCK TABLES;"],
        stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    check("profile/deck DB lock acquired",locker.stdout.readline().strip()==b"locked")
    q.sendall(frame(10,field(quoted,32),qt)+frame(17,struct.pack("<ii",0,4),qt))
    time.sleep(.1)
    start=time.monotonic();q.sendall(frame(254))
    check("ping remains responsive during blocked profile/deck",
          receive(q)[0]==255 and time.monotonic()-start < .8)
    locker.wait(timeout=5)
    check("profile/deck complete after unlock",receive(q)[0]==11 and receive(q)[0]==18)
    server.stdin.write(b"q\n");server.stdin.flush()
    server.wait(timeout=8)
    check("clean server shutdown",server.returncode==0)
    print(f"PASS: {len(checks)} integration assertions; synthetic runtime: {runtime}")
finally:
    for s in sockets: s.close()
    for p in reversed(processes):
        if p.poll() is None:
            p.terminate()
            try:p.wait(timeout=5)
            except subprocess.TimeoutExpired:p.kill();p.wait()
    for log in logs:log.close()
