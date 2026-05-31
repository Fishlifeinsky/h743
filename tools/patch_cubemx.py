#!/usr/bin/env python3
"""CubeMX 生成后自动补丁 — 避免 CubeMX 覆盖导致编译/链接错误

每次 CubeMX 重新生成代码后运行此脚本:
    python tools/patch_cubemx.py

它做两件事:
1. 修复 CMakeLists.txt 中空的 -mfpu= 变量
2. 守卫 stm32h7xx_it.c 中与 FreeRTOS 冲突的 SVC/PendSV/SysTick 处理器
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def patch_cmakelists():
    """修复 CMakeLists.txt: -mfpu=${mfpu} → -mfpu=fpv5-d16"""
    cmake = ROOT / "CMakeLists.txt"
    text = cmake.read_text(encoding="utf-8")

    patched = text.replace("-mfpu=${mfpu}", "-mfpu=fpv5-d16")
    patched = patched.replace("-mfpu=$(", "-mfpu=fpv5-d16")  # 防各种变体

    if patched != text:
        cmake.write_text(patched, encoding="utf-8")
        print("[OK] CMakeLists.txt: 已修复 -mfpu=fpv5-d16")
    else:
        print("[OK] CMakeLists.txt: 无需修复 (已经是 fpv5-d16)")


def patch_stm32h7xx_it():
    """守卫 stm32h7xx_it.c 中 FreeRTOS 冲突的 handler"""
    it_file = ROOT / "Core" / "Src" / "stm32h7xx_it.c"
    text = it_file.read_text(encoding="utf-8")

    handlers = ["SVC_Handler", "PendSV_Handler", "SysTick_Handler"]

    for func_name in handlers:
        # 检查是否已有守卫
        if re.search(rf"#if !BSP_FREERTOS_ENABLED\s*\n\s*void {func_name}", text):
            continue

        # 只匹配函数定义, 不包含前面的 @brief 注释 (避免 DOTALL 跨 handler 误匹配)
        pattern = re.compile(
            rf"void\s+{func_name}\s*\(void\)\s*\n"
            rf"\{{.*?\n\}}",
            re.DOTALL
        )
        match = pattern.search(text)
        if not match:
            print(f"[WARN] stm32h7xx_it.c: 找不到 {func_name} 函数定义")
            continue

        original = match.group(0)
        guarded = f"#if !BSP_FREERTOS_ENABLED\n{original}\n#endif"
        text = text.replace(original, guarded, 1)
        print(f"[OK] stm32h7xx_it.c: 已为 {func_name} 添加 BSP_FREERTOS_ENABLED 守卫")

    it_file.write_text(text, encoding="utf-8")


def main():
    print("=== CubeMX 生成后补丁 ===\n")
    patch_cmakelists()
    patch_stm32h7xx_it()
    print("\n完成.")


if __name__ == "__main__":
    main()
