# xorDLL // by skyfiend

**xorDLL** is a lightweight, native C++ DLL injector for Windows featuring both a modern GUI and a powerful CLI. It supports standard and advanced injection methods like `CreateRemoteThread` and `NtCreateThreadEx` with zero external dependencies.

## Usage

### GUI Mode
Simply launch `xorDLL.exe`, select a target process from the list, add your DLL files, and click **Inject**.

### CLI Mode
For automation or scripting, use the command line interface:
```bash
xorDLL_cli.exe -p <pid> -d <path_to_dll> -m <method>
```

## Bug Reporting

Found a bug or have a suggestion? Please open an issue on the [GitHub Issues](https://github.com/skyfiend/xorDLL/issues) page.
When reporting bugs, please include your Windows version and steps to reproduce the problem.

## Disclaimer & License

**EDUCATIONAL USE ONLY.** Do not use this software for malicious purposes.

Licensed under the [MIT License](LICENSE).
Developed by **skyfiend** (2025).

