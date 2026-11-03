from pathlib import Path
import re
import shutil
from datetime import datetime

path = Path("src/client.c")
s = path.read_text()

# Sao lưu trước khi sửa
backup = path.with_name(
    f"client.c.backup_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
)
shutil.copy2(path, backup)

old_globals = (
    "static Problem problems[256];static int problem_count=0;"
    "static GtkWidget*window,*host_entry,*user_entry,*pass_entry,*problem_combo,"
    "*description_view,*code_view,*result_label,*history_view,*status_label,"
    "*submit_button,*history_button;"
    "static char current_user[80]=\"\";static int logged_in=0;static int server_fd=-1;"
)
new_globals = (
    "static Problem problems[256];static int problem_count=0;"
    "static GtkWidget*window,*login_window,*login_status_box,*workspace_root;"
    "static GtkWidget*host_entry,*user_entry,*pass_entry,*problem_combo,"
    "*description_view,*code_view,*result_label,*history_view,*status_label,"
    "*submit_button,*history_button;"
    "static char current_user[80]=\"\";static int logged_in=0;static int server_fd=-1;"
)

if old_globals not in s:
    raise SystemExit("Khong tim thay khai bao bien cu. Chua sua file.")

s = s.replace(old_globals, new_globals, 1)

s = s.replace(
    '"Create CodeJudge Account",GTK_WINDOW(window),GTK_DIALOG_MODAL,',
    '"Create CodeJudge Account",GTK_WINDOW(login_window),GTK_DIALOG_MODAL,',
    1
)

old_login = (
    "static void on_login(GtkButton*b,gpointer x){"
    "(void)b;(void)x;if(authenticate()){load_problems();}}"
)
new_login = r'''
static void on_login(GtkButton*b,gpointer x){
 (void)b;(void)x;
 if(authenticate()){
  load_problems();

  /* Chuyen thanh trang thai tu man hinh dang nhap sang workspace */
  if(login_status_box && status_label &&
     gtk_widget_get_parent(status_label)==login_status_box){
   gtk_container_remove(GTK_CONTAINER(login_status_box),status_label);
   gtk_box_pack_start(GTK_BOX(workspace_root),status_label,FALSE,FALSE,0);
  }

  gtk_widget_show_all(window);
  gtk_widget_hide(login_window);
 }
}
'''
if old_login not in s:
    raise SystemExit("Khong tim thay ham on_login cu. Chua sua file.")
s = s.replace(old_login, new_login, 1)

new_tail = r'''
static void on_toggle_password(GtkToggleButton *button,gpointer data){
 (void)data;
 gtk_entry_set_visibility(GTK_ENTRY(pass_entry),
                          gtk_toggle_button_get_active(button));
}

int main(int argc,char**argv){
 gtk_init(&argc,&argv);
 load_css();

 /* ================= WORKSPACE ================= */
 window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
 gtk_window_set_title(GTK_WINDOW(window),"CodeJudge | Student Workspace");
 gtk_window_set_default_size(GTK_WINDOW(window),1160,790);
 gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
 g_signal_connect(window,"destroy",G_CALLBACK(on_window_destroy),NULL);

 GtkWidget*root=gtk_box_new(GTK_ORIENTATION_VERTICAL,12);
 workspace_root=root;
 gtk_container_set_border_width(GTK_CONTAINER(root),18);
 gtk_container_add(GTK_CONTAINER(window),root);

 GtkWidget*head=gtk_box_new(GTK_ORIENTATION_VERTICAL,3);
 GtkWidget*t=gtk_label_new("CODEJUDGE  /  STUDENT WORKSPACE");
 gtk_widget_set_halign(t,GTK_ALIGN_START);
 gtk_style_context_add_class(gtk_widget_get_style_context(t),"hero");

 GtkWidget*sub=gtk_label_new(
  "Solve programming problems, write C code and receive instant evaluation."
 );
 gtk_widget_set_halign(sub,GTK_ALIGN_START);
 gtk_style_context_add_class(gtk_widget_get_style_context(sub),"muted");
 gtk_box_pack_start(GTK_BOX(head),t,FALSE,FALSE,0);
 gtk_box_pack_start(GTK_BOX(head),sub,FALSE,FALSE,0);
 gtk_box_pack_start(GTK_BOX(root),head,FALSE,FALSE,0);

 /* Problem list and statement */
 GtkWidget*mainrow=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,14);
 gtk_widget_set_vexpand(mainrow,TRUE);

 GtkWidget*left=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);
 gtk_widget_set_size_request(left,390,-1);
 gtk_box_pack_start(GTK_BOX(mainrow),left,FALSE,FALSE,0);
 gtk_box_pack_start(GTK_BOX(left),left_label("PROBLEM LIST"),FALSE,FALSE,0);

 problem_combo=gtk_combo_box_text_new();
 g_signal_connect(problem_combo,"changed",G_CALLBACK(on_problem_changed),NULL);

 GtkWidget*problembar=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);
 gtk_box_pack_start(GTK_BOX(problembar),problem_combo,TRUE,TRUE,0);

 GtkWidget*refreshproblems=gtk_button_new_with_label("↻");
 gtk_widget_set_tooltip_text(refreshproblems,"Refresh problem list from server");
 g_signal_connect(refreshproblems,"clicked",
                  G_CALLBACK(on_refresh_problems),NULL);
 gtk_box_pack_start(GTK_BOX(problembar),refreshproblems,FALSE,FALSE,0);
 gtk_box_pack_start(GTK_BOX(left),problembar,FALSE,FALSE,0);
 gtk_box_pack_start(GTK_BOX(left),left_label("PROBLEM STATEMENT"),FALSE,FALSE,0);

 description_view=gtk_text_view_new();
 gtk_text_view_set_editable(GTK_TEXT_VIEW(description_view),FALSE);
 gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(description_view),GTK_WRAP_WORD_CHAR);
 gtk_box_pack_start(GTK_BOX(left),scroll(description_view),TRUE,TRUE,0);

 /* C source editor */
 GtkWidget*right=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);
 gtk_widget_set_hexpand(right,TRUE);
 gtk_box_pack_start(GTK_BOX(mainrow),right,TRUE,TRUE,0);

 GtkWidget*codehead=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);
 gtk_box_pack_start(GTK_BOX(codehead),left_label("C SOURCE EDITOR"),TRUE,TRUE,0);

 GtkWidget*open=gtk_button_new_with_label("Open .c file");
 g_signal_connect(open,"clicked",G_CALLBACK(on_choose_file),NULL);
 gtk_box_pack_end(GTK_BOX(codehead),open,FALSE,FALSE,0);
 gtk_box_pack_start(GTK_BOX(right),codehead,FALSE,FALSE,0);

 code_view=gtk_text_view_new();
 gtk_text_view_set_monospace(GTK_TEXT_VIEW(code_view),TRUE);
 gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(code_view),GTK_WRAP_NONE);
 gtk_box_pack_start(GTK_BOX(right),scroll(code_view),TRUE,TRUE,0);

 set_text(code_view,
  "#include <stdio.h>\n\n"
  "int main(void) {\n"
  "    int a, b;\n"
  "    scanf(\"%d %d\", &a, &b);\n"
  "    printf(\"%d\\n\", a + b);\n"
  "    return 0;\n"
  "}\n"
 );

 GtkWidget*act=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);
 submit_button=gtk_button_new_with_label("▶ Submit Solution");
 gtk_widget_set_sensitive(submit_button,FALSE);
 g_signal_connect(submit_button,"clicked",G_CALLBACK(on_submit),NULL);

 history_button=gtk_button_new_with_label("Refresh Submission History");
 g_signal_connect(history_button,"clicked",G_CALLBACK(on_history),NULL);

 gtk_box_pack_start(GTK_BOX(act),submit_button,FALSE,FALSE,0);
 gtk_box_pack_start(GTK_BOX(act),history_button,FALSE,FALSE,0);
 gtk_box_pack_start(GTK_BOX(right),act,FALSE,FALSE,0);
 gtk_box_pack_start(GTK_BOX(root),mainrow,TRUE,TRUE,0);

 /* Result and submission history */
 GtkWidget*bottom=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,14);
 GtkWidget*res=gtk_box_new(GTK_ORIENTATION_VERTICAL,6);
 GtkWidget*hist=gtk_box_new(GTK_ORIENTATION_VERTICAL,6);
 gtk_widget_set_hexpand(res,TRUE);
 gtk_widget_set_hexpand(hist,TRUE);
 gtk_box_pack_start(GTK_BOX(bottom),res,TRUE,TRUE,0);
 gtk_box_pack_start(GTK_BOX(bottom),hist,TRUE,TRUE,0);

 gtk_box_pack_start(GTK_BOX(res),left_label("LATEST RESULT"),FALSE,FALSE,0);
 result_label=gtk_label_new(
  "Submit code to view AC / WA / CE / RE / TLE and score."
 );
 gtk_widget_set_halign(result_label,GTK_ALIGN_START);
 gtk_label_set_line_wrap(GTK_LABEL(result_label),TRUE);
 gtk_label_set_selectable(GTK_LABEL(result_label),TRUE);
 gtk_box_pack_start(GTK_BOX(res),result_label,FALSE,FALSE,0);

 gtk_box_pack_start(GTK_BOX(hist),left_label("SUBMISSION HISTORY"),FALSE,FALSE,0);
 history_view=gtk_text_view_new();
 gtk_text_view_set_editable(GTK_TEXT_VIEW(history_view),FALSE);
 gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(history_view),GTK_WRAP_WORD_CHAR);

 GtkWidget*hs=scroll(history_view);
 gtk_widget_set_size_request(hs,-1,130);
 gtk_box_pack_start(GTK_BOX(hist),hs,TRUE,TRUE,0);
 gtk_box_pack_start(GTK_BOX(root),bottom,FALSE,FALSE,0);

 /* ================= LOGIN WINDOW ================= */
 login_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
 gtk_window_set_title(GTK_WINDOW(login_window),"CodeJudge | Sign In");
 gtk_window_set_default_size(GTK_WINDOW(login_window),480,650);
 gtk_window_set_resizable(GTK_WINDOW(login_window),FALSE);
 gtk_window_set_position(GTK_WINDOW(login_window),GTK_WIN_POS_CENTER);
 g_signal_connect(login_window,"destroy",G_CALLBACK(on_window_destroy),NULL);

 GtkWidget*outer=gtk_box_new(GTK_ORIENTATION_VERTICAL,18);
 gtk_container_set_border_width(GTK_CONTAINER(outer),32);
 gtk_container_add(GTK_CONTAINER(login_window),outer);

 GtkWidget*brand=gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
 gtk_widget_set_halign(brand,GTK_ALIGN_CENTER);

 GtkWidget*logo=gtk_label_new("CJ");
 gtk_style_context_add_class(gtk_widget_get_style_context(logo),"login-logo");
 gtk_box_pack_start(GTK_BOX(brand),logo,FALSE,FALSE,0);

 GtkWidget*brandname=gtk_label_new("CODEJUDGE");
 gtk_style_context_add_class(gtk_widget_get_style_context(brandname),"login-brand");
 gtk_box_pack_start(GTK_BOX(brand),brandname,FALSE,FALSE,0);

 GtkWidget*tagline=gtk_label_new("ONLINE JUDGE PLATFORM");
 gtk_style_context_add_class(gtk_widget_get_style_context(tagline),"muted");
 gtk_box_pack_start(GTK_BOX(brand),tagline,FALSE,FALSE,0);
 gtk_box_pack_start(GTK_BOX(outer),brand,FALSE,FALSE,8);

 GtkWidget*heading=gtk_label_new("Welcome back");
 gtk_style_context_add_class(gtk_widget_get_style_context(heading),"login-heading");
 gtk_widget_set_halign(heading,GTK_ALIGN_START);
 gtk_box_pack_start(GTK_BOX(outer),heading,FALSE,FALSE,0);

 GtkWidget*intro=gtk_label_new("Sign in to continue to your coding workspace.");
 gtk_style_context_add_class(gtk_widget_get_style_context(intro),"muted");
 gtk_widget_set_halign(intro,GTK_ALIGN_START);
 gtk_box_pack_start(GTK_BOX(outer),intro,FALSE,FALSE,0);

 GtkWidget*card=gtk_box_new(GTK_ORIENTATION_VERTICAL,10);
 gtk_style_context_add_class(gtk_widget_get_style_context(card),"login-card");
 gtk_container_set_border_width(GTK_CONTAINER(card),20);
 gtk_box_pack_start(GTK_BOX(outer),card,TRUE,TRUE,0);

 GtkWidget*serverlabel=left_label("SERVER IP ADDRESS");
 gtk_style_context_add_class(gtk_widget_get_style_context(serverlabel),"field-label");
 gtk_box_pack_start(GTK_BOX(card),serverlabel,FALSE,FALSE,0);

 host_entry=gtk_entry_new();
 gtk_entry_set_text(GTK_ENTRY(host_entry),"127.0.0.1");
 gtk_entry_set_placeholder_text(GTK_ENTRY(host_entry),"127.0.0.1");
 gtk_box_pack_start(GTK_BOX(card),host_entry,FALSE,FALSE,0);

 GtkWidget*userlabel=left_label("USERNAME");
 gtk_style_context_add_class(gtk_widget_get_style_context(userlabel),"field-label");
 gtk_box_pack_start(GTK_BOX(card),userlabel,FALSE,FALSE,0);

 user_entry=gtk_entry_new();
 gtk_entry_set_text(GTK_ENTRY(user_entry),"student");
 gtk_entry_set_placeholder_text(GTK_ENTRY(user_entry),"Enter your username");
 gtk_box_pack_start(GTK_BOX(card),user_entry,FALSE,FALSE,0);

 GtkWidget*passlabel=left_label("PASSWORD");
 gtk_style_context_add_class(gtk_widget_get_style_context(passlabel),"field-label");
 gtk_box_pack_start(GTK_BOX(card),passlabel,FALSE,FALSE,0);

 pass_entry=gtk_entry_new();
 gtk_entry_set_visibility(GTK_ENTRY(pass_entry),FALSE);
 gtk_entry_set_placeholder_text(GTK_ENTRY(pass_entry),"Enter your password");
 gtk_entry_set_activates_default(GTK_ENTRY(pass_entry),TRUE);
 gtk_box_pack_start(GTK_BOX(card),pass_entry,FALSE,FALSE,0);

 GtkWidget*showpass=gtk_check_button_new_with_label("Show password");
 g_signal_connect(showpass,"toggled",G_CALLBACK(on_toggle_password),NULL);
 gtk_widget_set_halign(showpass,GTK_ALIGN_START);
 gtk_box_pack_start(GTK_BOX(card),showpass,FALSE,FALSE,2);

 GtkWidget*loginbtn=gtk_button_new_with_label("SIGN IN");
 gtk_widget_set_name(loginbtn,"primary-login");
 gtk_widget_set_size_request(loginbtn,-1,46);
 g_signal_connect(loginbtn,"clicked",G_CALLBACK(on_login),NULL);
 gtk_box_pack_start(GTK_BOX(card),loginbtn,FALSE,FALSE,8);

 GtkWidget*registerbtn=gtk_button_new_with_label("CREATE ACCOUNT");
 gtk_widget_set_size_request(registerbtn,-1,42);
 g_signal_connect(registerbtn,"clicked",G_CALLBACK(on_register),NULL);
 gtk_box_pack_start(GTK_BOX(card),registerbtn,FALSE,FALSE,0);

 login_status_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,4);
 status_label=gtk_label_new("Please sign in to access the problem list.");
 gtk_label_set_line_wrap(GTK_LABEL(status_label),TRUE);
 gtk_widget_set_halign(status_label,GTK_ALIGN_CENTER);
 gtk_box_pack_start(GTK_BOX(login_status_box),status_label,FALSE,FALSE,0);
 gtk_box_pack_start(GTK_BOX(outer),login_status_box,FALSE,FALSE,0);

 GtkWidget*foot=gtk_label_new("CodeJudge  •  C Programming Practice");
 gtk_style_context_add_class(gtk_widget_get_style_context(foot),"muted");
 gtk_box_pack_start(GTK_BOX(outer),foot,FALSE,FALSE,0);

 /* Chi hien man hinh dang nhap khi khoi dong */
 gtk_widget_show_all(login_window);
 gtk_main();
 return 0;
}
'''

s = s[:s.index("int main(")] + new_tail

# Them style cho man hinh dang nhap
needle = '"separator{background:#263449}";'
extra = (
 '"separator{background:#263449}"'
 '".login-logo{background:#263b78;color:#ffffff;font-size:25pt;'
 'font-weight:900;padding:13px 20px;border-radius:16px}"'
 '".login-brand{font-size:21pt;font-weight:900;color:#f8fafc;letter-spacing:2px}"'
 '".login-heading{font-size:19pt;font-weight:800;color:#f8fafc}"'
 '".login-card{background:#111c2e;border:1px solid #293a55;border-radius:14px}"'
 '".field-label{color:#aab9d0;font-size:9pt;font-weight:700}"'
 '#primary-login{background:#405bd5;border:1px solid #6178ed;'
 'border-radius:9px;font-size:11pt;font-weight:800;padding:10px}'
 '#primary-login:hover{background:#526deb}'
)
if needle not in s:
    raise SystemExit("Khong tim thay vi tri CSS. Da tao backup, file chua duoc ghi.")
s = s.replace(needle, extra, 1)

path.write_text(s)
print("Da cap nhat giao dien dang nhap.")
print("Ban sao luu:", backup)
