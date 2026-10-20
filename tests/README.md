# Server regression tests

These tests use synthetic accounts and a new local MariaDB data directory. They
do not use production credentials, run migrations, or connect to a deployed game.
Windows, Visual Studio 2022 C++ tools, Python 3, CMake 3.22+ and a MariaDB Windows
ZIP package with client headers/library are required.

From the repository root in a Visual Studio developer shell:

```powershell
$dbRoot = 'C:\path\to\mariadb-11.4.4-winx64'
msbuild projectMagicWar/projectMagicWar.vcxproj /p:Configuration=Debug /p:Platform=x64 "/p:MariaDBRoot=$dbRoot"
cmake -S tests -B build/tests -A x64 "-DMariaDBRoot=$dbRoot"
cmake --build build/tests --config Debug
ctest --test-dir build/tests -C Debug --output-on-failure
python tests/integration.py --mariadb-root $dbRoot --server projectMagicWar/x64/Debug/projectMagicWar.exe --runtime-dir build/integration-new
```

The integration runtime directory must not already exist. Choose a new directory
for each run. The test starts hidden child processes on loopback with temporary
ports, retains diagnostic logs and the synthetic database, and stops its child
processes on completion/failure. No Windows service is installed.

The worker tests cover FIFO ordering, bounded queue rejection, shutdown draining,
task exceptions, restart, obsolete session generations, disconnect cancellation,
and owned timer cancellation. The integration test covers fragmented/coalesced
frames, the header-inclusive frame limit, malformed string fields, signup/login,
profile/deck queries, ordered deck replacement, duplicate login, quote/backslash
SQL parameters, responsiveness during real DB locks, and logout during a pending
login. Assertions are also enabled in the Release worker-test build.

This is not a full Unity gameplay, load, or production-schema migration test.
