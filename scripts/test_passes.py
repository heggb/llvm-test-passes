#!/usr/bin/env python3
import argparse, subprocess, sys, os, time
from pathlib import Path

# Автодетект путей
SCRIPT_DIR   = Path(__file__).resolve().parent                    
PROJECT_DIR  = SCRIPT_DIR.parent                                  
LLVM_ROOT    = PROJECT_DIR.parent                                 

SRC_DIR      = PROJECT_DIR/"tests_src"                          
IR_DIR       = PROJECT_DIR/"build"/"ir"                       
RESULTS_DIR  = PROJECT_DIR/"results"                            
PASSES_DIR   = PROJECT_DIR/"build"                              

# Бинарники инструментария (
CLANG        = LLVM_ROOT/"build"/"bin"/"clang-22"
OPT          = LLVM_ROOT/"build"/"bin"/"opt"

# Конфиг проходов
PASS_CONFIG = {
    "MyPass":         {"plugin": "libTestPasses.so", "mode": "new", "pipeline": "MyPass"},
    "RPOPass":        {"plugin": "libTestPasses.so", "mode": "new", "pipeline": "RPOPass"},
    "InstSearchPass": {"plugin": "libTestPasses.so", "mode": "new", "pipeline": "InstSearchPass"},
}

# Утилиты 
def cprint(msg, color=None, attrs=None):
    print(msg)

def ensure_dirs():
    IR_DIR.mkdir(parents=True, exist_ok=True)

def compile_to_ir(src_file: Path) -> Path:
    """Скомпилировать C → LLVM IR (.ll)."""
    ensure_dirs()
    ir_file = IR_DIR / (src_file.stem + ".ll")
    cmd = [
        str(CLANG), "-O0", "-S", "-emit-llvm",
        str(src_file), "-o", str(ir_file),
    ]
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if proc.returncode != 0:
        raise RuntimeError(f"clang failed for {src_file.name}:\n{proc.stderr}")
    return ir_file

def run_opt(ir_file: Path, pass_name: str, stream: str = "stderr"):
    """
    stream: 'stderr' | 'stdout' | 'both'
    Возвращает текст указанного потока(ов) и время в мс.
    """
    cfg = PASS_CONFIG.get(pass_name)
    if not cfg:
        raise KeyError(f"Unknown pass '{pass_name}'. Known: {', '.join(PASS_CONFIG)}")

    plugin_path = PASSES_DIR / cfg["plugin"]
    if not plugin_path.exists():
        raise FileNotFoundError(f"Plugin not found: {plugin_path}")

    pipeline = cfg["pipeline"]
    if cfg.get("mode", "new") == "new":
        cmd = [
            str(OPT),
            f"-load-pass-plugin={plugin_path}",
            f"-passes={pipeline}",
            "-disable-output",
            str(ir_file),
        ]
    else:
        cmd = [
            str(OPT),
            f"-load={plugin_path}",
            f"-{pipeline}",
            "-disable-output",
            str(ir_file),
        ]

    start = time.time()
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    elapsed_ms = (time.time() - start) * 1000
    if proc.returncode != 0:
        raise RuntimeError(f"opt failed for {ir_file.name} with {pass_name}:\n{proc.stderr}")

    if stream == "stdout":
        out = proc.stdout
    elif stream == "both":
        out = (proc.stdout or "") + (proc.stderr or "")
    else:
        out = proc.stderr  # по умолчанию — llvm::errs()
    return out, elapsed_ms

def expected_output_path(pass_name: str, src_file: Path) -> Path:
    return RESULTS_DIR / pass_name / (src_file.stem + ".txt")

def compare_text(actual: str, expected: str):
    a = [l.rstrip() for l in actual.splitlines()]
    e = [l.rstrip() for l in expected.splitlines()]
    for i, exp in enumerate(e):
        if i >= len(a) or a[i] != exp:
            got = a[i] if i < len(a) else "<no output>"
            return False, f"Mismatch at line {i+1}\nExpected: {exp}\nGot     : {got}\n"
    if len(a) > len(e):
        return False, f"Extra output starting at line {len(e)+1}: {a[len(e)]}"
    return True, ""

def run_single(pass_name: str, c_filename: str) -> bool:
    src = (SRC_DIR / c_filename).resolve()
    if not src.exists():
        cprint(f"✖ Source not found: {src}")
        return False
    try:
        ir = compile_to_ir(src)
        out_text, elapsed_ms = run_opt(ir, pass_name, stream="stderr")
    except Exception as e:
        cprint(f"✖ Error: {e}")
        return False

    exp_path = expected_output_path(pass_name, src)
    if not exp_path.exists():
        cprint(f"✖ Expected output not found: {exp_path}")
        cprint("Tip: run with --update to create baseline.")
        return False

    ok, msg = compare_text(out_text.strip(), exp_path.read_text(encoding="utf-8").strip())
    name = f"{pass_name}:{src.name}"
    if ok:
        cprint(f"{name}: OK ({elapsed_ms:.1f} ms)")
        return True
    else:
        cprint(f"\n{name}: MISMATCH")
        print(msg)
        return False

def discover_tests(only_pass: str | None):
    items = []
    for c in sorted(SRC_DIR.glob("*.c")):
        if only_pass:
            items.append((only_pass, c.name))
        else:
            for p in PASS_CONFIG.keys():
                items.append((p, c.name))
    return items

def main():
    ap = argparse.ArgumentParser(description="LLVM test passes runner (auto-paths)")
    ap.add_argument("pass_name", nargs="?", help="MyPass | RPOPass | InstSearchPass")
    ap.add_argument("c_file",    nargs="?", help="input.c from tests_src")
    ap.add_argument("--update", action="store_true", help="Write current stdout to results/<pass>/<file>.txt")
    args = ap.parse_args()

    if args.pass_name and args.c_file:
        scope = [(args.pass_name, args.c_file)]
    elif args.pass_name:
        scope = discover_tests(args.pass_name)
    else:
        scope = discover_tests(None)

    if not scope:
        cprint("No tests discovered.")
        sys.exit(2)

    total = passed = 0
    for p, c in scope:
        total += 1
        if args.update:
            try:
                ir = compile_to_ir(SRC_DIR / c)
                out_text, _ = run_opt(ir, p, stream="stderr")
                out_path = expected_output_path(p, SRC_DIR / c)
                out_path.parent.mkdir(parents=True, exist_ok=True)
                out_path.write_text(out_text.strip() + "\n", encoding="utf-8")
                cprint(f"Updated baseline: {out_path}")
                passed += 1
            except Exception as e:
                cprint(f"Failed to update {p}:{c}: {e}")
        else:
            if run_single(p, c): passed += 1

    print()
    if passed == total:
        cprint(f"✅ All {total} test(s) passed")
        sys.exit(0)
    else:
        cprint(f"❌ {total - passed} of {total} test(s) failed")
        sys.exit(1)

if __name__ == "__main__":
    main()