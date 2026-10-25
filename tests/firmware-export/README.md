# Native firmware export check

These fixtures compile the actual `export_json.h` against small Arduino/SQLite stand-ins. They check 3,000 streamed rows while C++ allocation is forbidden, then parse every frame with Node. Coverage includes Unicode, quotes, slashes, embedded control/NUL characters, null text, 64-bit IDs, fallback timestamps, money, and cursor errors. They also exercise the acknowledgment guard with unchanged rows, newly added rows, edited values, a failed update, and a missing successful export. SQLite is a stand-in here; compilation and physical device checks are still needed.

In a **Developer Command Prompt for VS 2022**, change to a temporary build directory and run these commands (adjust the repository path if moved):

```bat
cl /nologo /std:c++17 /EHsc /utf-8 /I"C:\Users\ProfessorS3rgio\Documents\Arduino\Watersystem\tests\firmware-export" "C:\Users\ProfessorS3rgio\Documents\Arduino\Watersystem\tests\firmware-export\export-json.cpp" /Fe:export-json.exe
export-json.exe > output.jsonl
node "C:\Users\ProfessorS3rgio\Documents\Arduino\Watersystem\tests\firmware-export\verify.mjs" output.jsonl
```

Each command must exit successfully. Keep generated executables, object files, and the output fixture outside the repository.
