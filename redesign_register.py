from pathlib import Path
from datetime import datetime
import shutil

path = Path("src/client.c")
src = path.read_text()

backup = path.with_name(
    "client.c.backup_register_" + datetime.now().strftime("%Y%m%d_%H%M%S")
)
shutil.copy2(path, backup)

# Them cac bien giao dien dang ky
marker = 'static char current_user[80]="";'
if "register_window" not in src:
    src = src.replace(
        marker,
        marker + '''
static GtkWidget *register_window;
static GtkWidget *reg_name_entry, *reg_user_entry;
static GtkWidget *reg_pass_entry, *reg_confirm_entry;
static GtkWidget *reg_status_label;
''',
        1
    )

start = src.find("static void toggle_register_password(")
end = src.find("static void on_login(", start)

if start < 0 or end < 0:
    raise SystemExit("Khong tim thay vi tri ham dang ky. Khong sua file.")

new_code = r'''
static void toggle_register_password(GtkToggleButton *button,gpointer data){
    gtk_entry_set_visibility(
        GTK_ENTRY(data),
        gtk_toggle_button_get_active(button)
    );
}

static GtkWidget *register_field(GtkWidget *parent,
                                 const char *label_text,
                                 const char *placeholder,
                                 int max_length,
                                 gboolean password){
    GtkWidget *label = gtk_label_new(label_text);
    gtk_style_context_add_class(
        gtk_widget_get_style_context(label), "field-label"
    );
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), placeholder);
    gtk_entry_set_max_length(GTK_ENTRY(entry), max_length);
    gtk_entry_set_visibility(GTK_ENTRY(entry), !password);
    gtk_widget_set_size_request(entry, -1, 42);
    gtk_box_pack_start(GTK_BOX(parent), entry, FALSE, FALSE, 0);

    return entry;
}

static void register_show_status(const char *message){
    if(reg_status_label)
        gtk_label_set_text(GTK_LABEL(reg_status_label), message);
}

static void on_register_submit(GtkButton *button,gpointer data){
    (void)button;
    (void)data;

    const char *n = gtk_entry_get_text(GTK_ENTRY(reg_name_entry));
    const char *u = gtk_entry_get_text(GTK_ENTRY(reg_user_entry));
    const char *pw = gtk_entry_get_text(GTK_ENTRY(reg_pass_entry));
    const char *q = gtk_entry_get_text(GTK_ENTRY(reg_confirm_entry));

    if(!*n || !*u || !*pw || !*q){
        register_show_status("Please fill in all fields.");
        return;
    }

    if(strlen(n)>80 || strlen(u)<3 || strlen(u)>32 ||
       strlen(pw)<6 || strlen(pw)>128){
        register_show_status(
            "Name max 80, username 3-32, password 6-128 characters."
        );
        return;
    }

    if(strcmp(pw,q)!=0){
        register_show_status("Password confirmation does not match.");
        return;
    }

    if(strchr(n,'|') || strchr(u,'|') || strchr(pw,'|') ||
       strchr(n,'\n') || strchr(u,'\n') || strchr(pw,'\n')){
        register_show_status("The character | is not allowed.");
        return;
    }

    int fd = connect_server();
    if(fd < 0){
        register_show_status("Cannot connect to Server. Check Server IP.");
        return;
    }

    char req[512], resp[OJ_MAX_LINE];
    snprintf(req, sizeof req, "REGISTER|%s|%s|%s\n", u, pw, n);

    if(send_all(fd, req, strlen(req)) == 0 &&
       recv_line(fd, resp, sizeof resp) > 0){

        if(!strcmp(resp, "REGISTER|OK")){
            gtk_entry_set_text(GTK_ENTRY(user_entry), u);
            gtk_entry_set_text(GTK_ENTRY(pass_entry), "");

            gtk_label_set_text(
                GTK_LABEL(status_label),
                "Registration successful. Please sign in."
            );

            register_show_status("Account created successfully!");
            close(fd);

            GtkWidget *old_window = register_window;
            register_window = NULL;
            gtk_widget_destroy(old_window);
            return;

        }else if(!strcmp(resp, "REGISTER|EXISTS")){
            register_show_status("Username already exists. Try another.");
        }else{
            register_show_status(
                "Registration failed. Check your information."
            );
        }
    }else{
        register_show_status("No response from Server.");
    }

    close(fd);
}

static void on_register_back(GtkButton *button,gpointer data){
    (void)button;
    (void)data;

    if(register_window){
        GtkWidget *old_window = register_window;
        register_window = NULL;
        gtk_widget_destroy(old_window);
    }
}

static void on_register(GtkButton *button,gpointer data){
    (void)button;
    (void)data;

    if(register_window){
        gtk_window_present(GTK_WINDOW(register_window));
        return;
    }

    register_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(
        GTK_WINDOW(register_window), "CodeJudge | Create Account"
    );
    gtk_window_set_default_size(GTK_WINDOW(register_window), 480, 760);
    gtk_window_set_resizable(GTK_WINDOW(register_window), FALSE);
    gtk_window_set_position(
        GTK_WINDOW(register_window), GTK_WIN_POS_CENTER_ON_PARENT
    );
    gtk_window_set_transient_for(
        GTK_WINDOW(register_window), GTK_WINDOW(login_window)
    );
    gtk_window_set_modal(GTK_WINDOW(register_window), TRUE);
    gtk_window_set_destroy_with_parent(
        GTK_WINDOW(register_window), TRUE
    );

    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 13);
    gtk_container_set_border_width(GTK_CONTAINER(outer), 25);
    gtk_container_add(GTK_CONTAINER(register_window), outer);

    /* Logo va thuong hieu giong trang dang nhap */
    GtkWidget *brand = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_halign(brand, GTK_ALIGN_CENTER);

    GtkWidget *logo = gtk_label_new("CJ");
    gtk_style_context_add_class(
        gtk_widget_get_style_context(logo), "login-logo"
    );
    gtk_box_pack_start(GTK_BOX(brand), logo, FALSE, FALSE, 0);

    GtkWidget *brandname = gtk_label_new("CODEJUDGE");
    gtk_style_context_add_class(
        gtk_widget_get_style_context(brandname), "login-brand"
    );
    gtk_box_pack_start(GTK_BOX(brand), brandname, FALSE, FALSE, 0);

    GtkWidget *tagline = gtk_label_new("ONLINE JUDGE PLATFORM");
    gtk_style_context_add_class(
        gtk_widget_get_style_context(tagline), "muted"
    );
    gtk_box_pack_start(GTK_BOX(brand), tagline, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(outer), brand, FALSE, FALSE, 5);

    GtkWidget *heading = gtk_label_new("Create your account");
    gtk_style_context_add_class(
        gtk_widget_get_style_context(heading), "login-heading"
    );
    gtk_widget_set_halign(heading, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(outer), heading, FALSE, FALSE, 0);

    GtkWidget *intro = gtk_label_new(
        "Join CodeJudge and start solving programming problems."
    );
    gtk_style_context_add_class(
        gtk_widget_get_style_context(intro), "muted"
    );
    gtk_widget_set_halign(intro, GTK_ALIGN_START);
    gtk_label_set_line_wrap(GTK_LABEL(intro), TRUE);
    gtk_box_pack_start(GTK_BOX(outer), intro, FALSE, FALSE, 0);

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(
        gtk_widget_get_style_context(card), "login-card"
    );
    gtk_container_set_border_width(GTK_CONTAINER(card), 17);
    gtk_box_pack_start(GTK_BOX(outer), card, TRUE, TRUE, 0);

    reg_name_entry = register_field(
        card, "FULL NAME", "Enter your full name", 80, FALSE
    );
    reg_user_entry = register_field(
        card, "USERNAME", "Choose a username", 32, FALSE
    );
    reg_pass_entry = register_field(
        card, "PASSWORD", "At least 6 characters", 128, TRUE
    );

    GtkWidget *show1 = gtk_check_button_new_with_label("Show password");
    gtk_widget_set_halign(show1, GTK_ALIGN_START);
    g_signal_connect(
        show1, "toggled",
        G_CALLBACK(toggle_register_password), reg_pass_entry
    );
    gtk_box_pack_start(GTK_BOX(card), show1, FALSE, FALSE, 0);

    reg_confirm_entry = register_field(
        card, "CONFIRM PASSWORD", "Re-enter your password", 128, TRUE
    );

    GtkWidget *show2 = gtk_check_button_new_with_label(
        "Show confirmation password"
    );
    gtk_widget_set_halign(show2, GTK_ALIGN_START);
    g_signal_connect(
        show2, "toggled",
        G_CALLBACK(toggle_register_password), reg_confirm_entry
    );
    gtk_box_pack_start(GTK_BOX(card), show2, FALSE, FALSE, 0);

    GtkWidget *note = gtk_label_new(
        "New accounts are registered with the STUDENT role."
    );
    gtk_style_context_add_class(
        gtk_widget_get_style_context(note), "muted"
    );
    gtk_label_set_line_wrap(GTK_LABEL(note), TRUE);
    gtk_widget_set_halign(note, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(card), note, FALSE, FALSE, 3);

    GtkWidget *create = gtk_button_new_with_label("CREATE ACCOUNT");
    gtk_widget_set_name(create, "primary-login");
    gtk_widget_set_size_request(create, -1, 46);
    g_signal_connect(
        create, "clicked", G_CALLBACK(on_register_submit), NULL
    );
    gtk_box_pack_start(GTK_BOX(card), create, FALSE, FALSE, 5);

    GtkWidget *back = gtk_button_new_with_label("BACK TO SIGN IN");
    gtk_widget_set_size_request(back, -1, 42);
    g_signal_connect(
        back, "clicked", G_CALLBACK(on_register_back), NULL
    );
    gtk_box_pack_start(GTK_BOX(card), back, FALSE, FALSE, 0);

    reg_status_label = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(reg_status_label), TRUE);
    gtk_widget_set_halign(reg_status_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(outer), reg_status_label, FALSE, FALSE, 0);

    GtkWidget *foot = gtk_label_new(
        "CodeJudge  •  C Programming Practice"
    );
    gtk_style_context_add_class(
        gtk_widget_get_style_context(foot), "muted"
    );
    gtk_box_pack_start(GTK_BOX(outer), foot, FALSE, FALSE, 0);

    g_signal_connect(
        register_window, "destroy",
        G_CALLBACK(gtk_widget_destroyed), &register_window
    );

    gtk_widget_show_all(register_window);
}
'''

src = src[:start] + new_code + "\n" + src[end:]
path.write_text(src)

print("Da cap nhat giao dien dang ky.")
print("File sao luu:", backup)
