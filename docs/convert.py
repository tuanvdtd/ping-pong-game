import re

with open('docs/PHAN_TICH_DEFENSE_MASTER.md', 'r', encoding='utf-8') as f:
    text = f.read()

# Try to use markdown module if installed, else fallback to custom
try:
    import markdown
    html_content = markdown.markdown(text, extensions=['tables', 'fenced_code'])
except ImportError:
    # Basic fallback
    html_content = text.replace('\n', '<br>')

html = f'''<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<title>Tài Liệu Bảo Vệ Đồ Án Ping Pong STM32</title>
<style>
body {{ font-family: 'Segoe UI', Arial, sans-serif; line-height: 1.8; max-width: 900px; margin: 0 auto; padding: 30px; color: #222; background-color: #ffffff; }}
h1, h2, h3 {{ color: #0056b3; margin-top: 1.5em; }}
code {{ background: #f4f4f4; padding: 2px 6px; border-radius: 4px; font-family: Consolas, monospace; font-size: 0.9em; }}
pre {{ background: #1e1e1e; color: #d4d4d4; padding: 15px; border-radius: 6px; overflow-x: auto; font-family: Consolas, monospace; }}
pre code {{ background: none; color: inherit; padding: 0; }}
blockquote {{ border-left: 4px solid #0056b3; margin: 15px 0; padding: 10px 15px; color: #444; background: #eef6ff; }}
table {{ border-collapse: collapse; width: 100%; margin: 20px 0; }}
th, td {{ border: 1px solid #ccc; padding: 10px 14px; text-align: left; }}
th {{ background-color: #0056b3; color: white; }}
tr:nth-child(even) {{ background-color: #f8f9fa; }}
</style>
</head>
<body>
{html_content}
</body>
</html>'''

with open('docs/PHAN_TICH_DEFENSE_MASTER.html', 'w', encoding='utf-8') as f:
    f.write(html)

print("SUCCESS")
