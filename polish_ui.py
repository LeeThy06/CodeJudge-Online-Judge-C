from pathlib import Path
from datetime import datetime
import shutil
import sys

client = Path("src/client.c")
server = Path("src/server.c")

stamp = datetime.now().strftime("%Y%m%d_%H%M%S")

for f in (client, server):
    if not f.exists():
        sys.exit(f"Không tìm thấy file: {f}")
    shutil.copy2(f, f"{f}.backup_ui_{stamp}")

def replace_once(content, old, new, filename):
    count = content.count(old)
    if count != 1:
        sys.exit(
            f"Lỗi: đoạn cần sửa trong {filename} xuất hiện {count} lần "
            f"(cần đúng 1 lần). Đã dừng, file gốc vẫn được giữ."
        )
    return content.replace(old, new, 1)

# ================= STUDENT CLIENT =================
c = client.read_text()

c = replace_once(
    c,
    'gtk_window_set_default_size(GTK_WINDOW(window),1160,790);',
    'gtk_window_set_default_size(GTK_WINDOW(window),1050,720);',
    'client.c'
)

c = replace_once(
    c,
    'gtk_container_set_border_width(GTK_CONTAINER(root),18);',
    'gtk_container_set_border_width(GTK_CONTAINER(root),16);',
    'client.c'
)

c = replace_once(
    c,
    'gtk_widget_set_vexpand(mainrow,TRUE);',
    'gtk_widget_set_vexpand(mainrow,FALSE);\n gtk_widget_set_size_request(mainrow,-1,420);',
    'client.c'
)

c = replace_once(
    c,
    'gtk_widget_set_size_request(left,390,-1);',
    'gtk_widget_set_size_request(left,350,-1);\n gtk_style_context_add_class(gtk_widget_get_style_context(left),"panel");\n gtk_container_set_border_width(GTK_CONTAINER(left),14);',
    'client.c'
)

c = replace_once(
    c,
    'gtk_widget_set_hexpand(right,TRUE);',
    'gtk_widget_set_hexpand(right,TRUE);\n gtk_style_context_add_class(gtk_widget_get_style_context(right),"panel");\n gtk_container_set_border_width(GTK_CONTAINER(right),14);',
    'client.c'
)

c = replace_once(
    c,
    'GtkWidget*bottom=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,14);',
    'GtkWidget*bottom=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,14);\n gtk_widget_set_size_request(bottom,-1,155);\n gtk_widget_set_vexpand(bottom,FALSE);',
    'client.c'
)

c = replace_once(
    c,
    'gtk_widget_set_hexpand(res,TRUE);',
    'gtk_widget_set_hexpand(res,TRUE);\n gtk_style_context_add_class(gtk_widget_get_style_context(res),"panel");\n gtk_container_set_border_width(GTK_CONTAINER(res),12);',
    'client.c'
)

c = replace_once(
    c,
    'gtk_widget_set_hexpand(hist,TRUE);',
    'gtk_widget_set_hexpand(hist,TRUE);\n gtk_style_context_add_class(gtk_widget_get_style_context(hist),"panel");\n gtk_container_set_border_width(GTK_CONTAINER(hist),12);',
    'client.c'
)

c = replace_once(
    c,
    'gtk_widget_set_size_request(hs,-1,130);',
    'gtk_widget_set_size_request(hs,-1,105);',
    'client.c'
)

c = replace_once(
    c,
    '"separator{background:#263449}"',
    '"separator{background:#263449}"\n ".panel{background:#111c2e;border:1px solid #293a55;border-radius:12px;padding:8px}"',
    'client.c'
)

c = replace_once(
    c,
    '"textview{background:#0a1020;color:#dbeafe;font-family:monospace;font-size:10pt;padding:10px}"',
    '"textview{background-color:#0a1020;color:#dbeafe;font-family:monospace;font-size:10pt;padding:10px}"',
    'client.c'
)

c = replace_once(
    c,
    '"textview text{background:#0a1020;color:#dbeafe}"',
    '"textview text{background-color:#0a1020;color:#dbeafe}"',
    'client.c'
)

client.write_text(c)

# ================= ADMIN SERVER =================
s = server.read_text()

s = replace_once(
    s,
    'gtk_window_set_default_size(GTK_WINDOW(window),1120,760);',
    'gtk_window_set_default_size(GTK_WINDOW(window),1050,700);',
    'server.c'
)

s = replace_once(
    s,
    '"textview{background:#0a1020;color:#dbeafe;font-family:monospace;padding:10px}"',
    '"textview{background-color:#0a1020;color:#dbeafe;font-family:monospace;padding:10px}"',
    'server.c'
)

s = replace_once(
    s,
    '"treeview{background:#111c2e;color:#e2e8f0}treeview:selected{background:#3446a5;color:#fff}"',
    '"textview text{background-color:#0a1020;color:#dbeafe}"\n "treeview{background:#111c2e;color:#e2e8f0}treeview:selected{background:#3446a5;color:#fff}"',
    'server.c'
)

s = replace_once(
    s,
    '"number{font-size:27px;font-weight:800;color:#9aaaff}"',
    '"number{font-size:27px;font-weight:800;color:#9aaaff}"\n ".card{background:#142137;border:1px solid #263957;border-radius:12px;padding:17px}"',
    'server.c'
)

server.write_text(s)

print("Đã chỉnh giao diện Student Workspace và Admin Console.")
print(f"Đã sao lưu file cũ với mã: {stamp}")
