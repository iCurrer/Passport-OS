#!/usr/bin/env python3
"""重新生成名牌的 GB2312 字库,使其与当前 LVGL 版本格式兼容。

背景:LVGL 8 与 9.5 的 lv_font_t.bitmap_format 语义不同(8:2=A4;9.5:2=COMPRESSED_NO_PREFILTER)。
旧版生成的字库在 LVGL 9.5 上会导致"所有文字空白、但布局/图片/声音正常"。
本脚本用 lv_font_conv 生成 bitmap_format=1(COMPRESSED)的字库,并同时包含
ASCII 0x20-0x7E + GB2312 6763 常用汉字。

用法(需要 node + npm 全局 lv_font_conv):
    python scripts/gen_badge_fonts.py

产物:
    main/badge_font_gb2312.c        (24px, 姓名)
    main/badge_font_gb2312_small.c  (14px, 顶部/职位/状态)
"""
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OTF = os.path.join(ROOT, "managed_components", "lvgl__lvgl", "scripts",
                   "built_in_font", "SourceHanSansSC-Normal.otf")
NODE = "C:/Program Files/nodejs/node.exe"
LV_FONT_CONV_CLI = "C:/Users/13783/AppData/Roaming/npm/node_modules/lv_font_conv/lib/cli.js"


def gb2312_chars():
    """GB2312 编码 6763 个常用汉字(区位 16~87)解码为 Unicode 字符串。"""
    chars = []
    for b1 in range(0xB0, 0xF8):
        for b2 in range(0xA1, 0x100):
            try:
                chars.append(bytes([b1, b2]).decode('gb2312'))
            except UnicodeDecodeError:
                pass
    return "".join(chars)


def q(s):
    """将字符串转成可安全嵌入 JS 字符串字面量的形式。"""
    return s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')


def main():
    symbols = gb2312_chars()
    chars_file = os.path.join(tempfile.gettempdir(), "gb2312_chars.txt")
    with open(chars_file, "w", encoding="utf-8") as f:
        f.write(symbols)

    js = r'''
const fs = require('fs');
const { run } = require('__LV_FONT_CONV_CLI__');
const sym = fs.readFileSync('__CHARS_FILE__', 'utf8').trim();
const otf = '__OTF__';
const jobs = [
  { size: 24, name: 'badge_font_gb2312', out: '__OUT_24__' },
  { size: 14, name: 'badge_font_gb2312_small', out: '__OUT_14__' },
];
(async () => {
  for (const j of jobs) {
    await run([
      '--font', otf,
      '--range', '0x20-0x7E',
      '--symbols', sym,
      '--size', String(j.size),
      '--bpp', '4',
      '--format', 'lvgl',
      '--lv-font-name', j.name,
      '--no-kerning',
      '-o', j.out,
    ]);
    // 把 lv_font_conv 的条件 include 固定为 "#include \"lvgl.h\""
    let src = fs.readFileSync(j.out, 'utf8');
    src = src.replace(/^#ifdef LV_LVGL_H_INCLUDE_SIMPLE[\s\S]*?#endif/m,
                      '#include "lvgl.h"');
    fs.writeFileSync(j.out, src);
    console.log('generated', j.name);
  }
})();
'''
    js = (js
          .replace('__LV_FONT_CONV_CLI__', q(LV_FONT_CONV_CLI))
          .replace('__CHARS_FILE__', q(chars_file))
          .replace('__OTF__', q(OTF))
          .replace('__OUT_24__', q(os.path.join(ROOT, 'main', 'badge_font_gb2312.c')))
          .replace('__OUT_14__', q(os.path.join(ROOT, 'main', 'badge_font_gb2312_small.c'))))

    js_file = os.path.join(tempfile.gettempdir(), "gen_fonts.js")
    with open(js_file, "w", encoding="utf-8") as f:
        f.write(js)

    print("char count:", len(symbols))
    subprocess.run([NODE, js_file], check=True)
    print("done: main/badge_font_gb2312.c, main/badge_font_gb2312_small.c")


if __name__ == "__main__":
    main()
