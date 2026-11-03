#include <gtk/gtk.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"

typedef struct{int id;char title[160],description[1200],input_desc[500],output_desc[500],sample_in[500],sample_out[500];int time_limit;}Problem;
static Problem problems[256];static int problem_count=0;static GtkWidget*window,*login_window,*login_status_box,*workspace_root;static GtkWidget*host_entry,*user_entry,*pass_entry,*problem_combo,*description_view,*code_view,*result_label,*history_view,*status_label,*submit_button,*history_button;static char current_user[80]="";
static GtkWidget *register_window;
static GtkWidget *reg_name_entry, *reg_user_entry;
static GtkWidget *reg_pass_entry, *reg_confirm_entry;
static GtkWidget *reg_status_label;
static int logged_in=0;static int server_fd=-1;
static char*decode(const char*s){gsize n=0;guchar*p=g_base64_decode(s,&n);char*r=g_malloc(n+1);memcpy(r,p,n);r[n]=0;g_free(p);return r;}
static int connect_server(void){const char*host=gtk_entry_get_text(GTK_ENTRY(host_entry));int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return -1;struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_port=htons(OJ_PORT);if(inet_pton(AF_INET,host,&a.sin_addr)!=1){close(fd);return -1;}struct timeval tv={10,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof tv);setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof tv);if(connect(fd,(struct sockaddr*)&a,sizeof a)<0){close(fd);return -1;}return fd;}
static void set_text(GtkWidget*w,const char*s){gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(w)),s?s:"",-1);}static char*get_text(GtkWidget*w){GtkTextBuffer*b=gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));GtkTextIter a,z;gtk_text_buffer_get_bounds(b,&a,&z);return gtk_text_buffer_get_text(b,&a,&z,FALSE);}static void history_append(const char*s){GtkTextBuffer*b=gtk_text_view_get_buffer(GTK_TEXT_VIEW(history_view));GtkTextIter e;gtk_text_buffer_get_end_iter(b,&e);gtk_text_buffer_insert(b,&e,s,-1);gtk_text_buffer_insert(b,&e,"\n",1);}
static void update_description(void){int i=gtk_combo_box_get_active(GTK_COMBO_BOX(problem_combo));if(i<0||i>=problem_count)return;Problem*p=&problems[i];char text[5000];snprintf(text,sizeof text,"%s\n\nINPUT\n%s\n\nOUTPUT\n%s\n\nSAMPLE INPUT\n%s\n\nSAMPLE OUTPUT\n%s\n\nTime limit: %d second(s)",p->description,p->input_desc,p->output_desc,p->sample_in,p->sample_out,p->time_limit);set_text(description_view,text);}
static void on_problem_changed(GtkComboBox*b,gpointer x){(void)b;(void)x;update_description();}
static int authenticate(void){if(server_fd>=0){close(server_fd);server_fd=-1;}const char*u=gtk_entry_get_text(GTK_ENTRY(user_entry)),*pw=gtk_entry_get_text(GTK_ENTRY(pass_entry));if(!*u||!*pw){gtk_label_set_text(GTK_LABEL(status_label),"Enter username and password first.");return 0;}int fd=connect_server();if(fd<0){gtk_label_set_text(GTK_LABEL(status_label),"Cannot connect. Check Server IP and make sure Server is running.");return 0;}char req[512];snprintf(req,sizeof req,"AUTH|%s|%s\n",u,pw);int ok=send_all(fd,req,strlen(req))==0;char line[OJ_MAX_LINE];if(ok&&recv_line(fd,line,sizeof line)>0&&strncmp(line,"AUTH|OK|",8)==0){char*role=decode(line+8);snprintf(current_user,sizeof current_user,"%s",u);logged_in=1;gtk_label_set_text(GTK_LABEL(status_label),!strcmp(role,"ADMIN")?"Signed in as ADMIN":"Signed in successfully");g_free(role);server_fd=fd;}else{logged_in=0;gtk_label_set_text(GTK_LABEL(status_label),"Login failed: invalid account or server response.");close(fd);}return logged_in;}
static void load_problems(void){if(!logged_in)return;int fd=server_fd;if(fd<0){gtk_label_set_text(GTK_LABEL(status_label),"Please login again to reconnect.");return;}send_all(fd,"LIST\n",5);gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(problem_combo));problem_count=0;char line[8192];while(recv_line(fd,line,sizeof line)>0){if(!strcmp(line,"END"))break;if(strncmp(line,"PROBLEM|",8))continue;char*save=NULL;strtok_r(line,"|",&save);char*id=strtok_r(NULL,"|",&save),*title=strtok_r(NULL,"|",&save),*desc=strtok_r(NULL,"|",&save),*input=strtok_r(NULL,"|",&save),*output=strtok_r(NULL,"|",&save),*si=strtok_r(NULL,"|",&save),*so=strtok_r(NULL,"|",&save),*tl=strtok_r(NULL,"|",&save);if(!id||!title||!desc||problem_count>=256)continue;Problem*p=&problems[problem_count++];p->id=atoi(id);char*v=decode(title);snprintf(p->title,sizeof p->title,"%s",v);g_free(v);v=decode(desc);snprintf(p->description,sizeof p->description,"%s",v);g_free(v);v=decode(input?input:"");snprintf(p->input_desc,sizeof p->input_desc,"%s",v);g_free(v);v=decode(output?output:"");snprintf(p->output_desc,sizeof p->output_desc,"%s",v);g_free(v);v=decode(si?si:"");snprintf(p->sample_in,sizeof p->sample_in,"%s",v);g_free(v);v=decode(so?so:"");snprintf(p->sample_out,sizeof p->sample_out,"%s",v);g_free(v);p->time_limit=tl?atoi(tl):2;char label[220];snprintf(label,sizeof label,"P%d  •  %s",p->id,p->title);gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(problem_combo),label);}if(problem_count){gtk_combo_box_set_active(GTK_COMBO_BOX(problem_combo),0);gtk_widget_set_sensitive(submit_button,TRUE);gtk_label_set_text(GTK_LABEL(status_label),"Connected • Problem list loaded");}else gtk_label_set_text(GTK_LABEL(status_label),"Connected • No problems available");}



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

static void on_refresh_problems(GtkButton*b,gpointer x){(void)b;(void)x;if(logged_in)load_problems();else gtk_label_set_text(GTK_LABEL(status_label),"Connect to the server before refreshing problems.");}
static void on_choose_file(GtkButton*b,gpointer x){(void)b;(void)x;GtkWidget*d=gtk_file_chooser_dialog_new("Open C source file",GTK_WINDOW(window),GTK_FILE_CHOOSER_ACTION_OPEN,"Cancel",GTK_RESPONSE_CANCEL,"Open",GTK_RESPONSE_ACCEPT,NULL);GtkFileFilter*f=gtk_file_filter_new();gtk_file_filter_set_name(f,"C source (*.c)");gtk_file_filter_add_pattern(f,"*.c");gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(d),f);if(gtk_dialog_run(GTK_DIALOG(d))==GTK_RESPONSE_ACCEPT){char*p=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d));gchar*contents=NULL;gsize n=0;GError*e=NULL;if(g_file_get_contents(p,&contents,&n,&e)){if(n<=OJ_MAX_CODE){set_text(code_view,contents);gtk_label_set_text(GTK_LABEL(status_label),"C source file loaded");}else gtk_label_set_text(GTK_LABEL(status_label),"File exceeds 256 KB");g_free(contents);}else{gtk_label_set_text(GTK_LABEL(status_label),e?e->message:"Cannot read file");if(e)g_error_free(e);}g_free(p);}gtk_widget_destroy(d);}
static void on_submit(GtkButton*b,gpointer x){(void)b;(void)x;if(!logged_in){gtk_label_set_text(GTK_LABEL(status_label),"Please login first.");return;}int i=gtk_combo_box_get_active(GTK_COMBO_BOX(problem_combo));if(i<0||i>=problem_count)return;char*code=get_text(code_view);size_t n=strlen(code);if(!n||n>OJ_MAX_CODE){gtk_label_set_text(GTK_LABEL(status_label),"Code is empty or larger than 256 KB");g_free(code);return;}int fd=server_fd;if(fd<0){gtk_label_set_text(GTK_LABEL(status_label),"Please login again to reconnect.");g_free(code);return;}char h[256];snprintf(h,sizeof h,"SUBMIT|%s|%d|%zu\n",current_user,problems[i].id,n);if(send_all(fd,h,strlen(h))<0||send_all(fd,code,n)<0){gtk_label_set_text(GTK_LABEL(status_label),"Failed to send submission; please login again.");close(fd);server_fd=-1;logged_in=0;g_free(code);return;}char line[OJ_MAX_LINE];if(recv_line(fd,line,sizeof line)>0){char st[24]="?",detail[512]="";int score=0;if(sscanf(line,"RESULT|%23[^|]|%d|%511[^\n]",st,&score,detail)>=2){char display[800];snprintf(display,sizeof display,"%s     %d / 10\n%s",st,score,detail);gtk_label_set_text(GTK_LABEL(result_label),display);char hist[900];snprintf(hist,sizeof hist,"Problem %d - %s - %d/10 - %s",problems[i].id,st,score,detail);history_append(hist);gtk_label_set_text(GTK_LABEL(status_label),"Submission judged and saved to history");}else gtk_label_set_text(GTK_LABEL(status_label),line);}else gtk_label_set_text(GTK_LABEL(status_label),"No response from Server; please login again.");g_free(code);}
static void on_history(GtkButton*b,gpointer x){(void)b;(void)x;if(!logged_in)return;int fd=server_fd;if(fd<0)return;char req[128];snprintf(req,sizeof req,"HISTORY|%s\n",current_user);send_all(fd,req,strlen(req));GtkTextBuffer*buf=gtk_text_view_get_buffer(GTK_TEXT_VIEW(history_view));gtk_text_buffer_set_text(buf,"",0);char line[2048];while(recv_line(fd,line,sizeof line)>0){if(!strcmp(line,"END"))break;if(strncmp(line,"HISTORY|",8))continue;char*save=NULL;strtok_r(line,"|",&save);char*sid=strtok_r(NULL,"|",&save),*pid=strtok_r(NULL,"|",&save),*title=strtok_r(NULL,"|",&save),*status=strtok_r(NULL,"|",&save),*score=strtok_r(NULL,"|",&save),*detail=strtok_r(NULL,"|",&save),*date=strtok_r(NULL,"|",&save);if(!sid||!title)continue;char*t=decode(title),*d=decode(detail?detail:"");char row[1200];snprintf(row,sizeof row,"#%s | P%s %s | %s | %s/10 | %s | %s",sid,pid?pid:"?",t,status?status:"?",score?score:"0",d,date?date:"");history_append(row);g_free(t);g_free(d);}}
static void on_window_destroy(GtkWidget*w,gpointer x){(void)w;(void)x;if(server_fd>=0)close(server_fd);gtk_main_quit();}
static GtkWidget*left_label(const char*s){GtkWidget*l=gtk_label_new(s);gtk_widget_set_halign(l,GTK_ALIGN_START);return l;}
static GtkWidget*scroll(GtkWidget*w){GtkWidget*s=gtk_scrolled_window_new(NULL,NULL);gtk_container_add(GTK_CONTAINER(s),w);gtk_widget_set_vexpand(s,TRUE);gtk_widget_set_hexpand(s,TRUE);return s;}
static void load_css(void){
 const char*css=
 "window{background:#0b1220;color:#e2e8f0}"
 "label{color:#e2e8f0;font-size:10pt}"
 ".hero{font-size:27px;font-weight:800;color:#f8fafc}"
 ".muted{color:#94a3b8;font-size:9pt}"
 ".section{font-size:11pt;font-weight:700;color:#cbd5e1}"
 "button{background:#3346a8;color:#fff;border:1px solid #5266d5;border-radius:8px;padding:9px 15px;font-weight:700}"
 "button:hover{background:#465bd0}button:disabled{background:#202b40;color:#64748b;border-color:#263449}"
 "entry,combobox{background:#111c2e;color:#f1f5f9;border:1px solid #334155;border-radius:7px;padding:7px;min-height:20px}"
 "entry:focus,combobox:focus{border:1px solid #818cf8}"
 "textview{background-color:#0a1020;color:#dbeafe;font-family:monospace;font-size:10pt;padding:10px}"
 "textview text{background-color:#0a1020;color:#dbeafe}"
 "scrolledwindow{border:1px solid #263449;border-radius:9px}"
 "treeview{background:#111c2e;color:#e2e8f0}treeview:selected{background:#3446a5;color:#fff}"
 "treeview header button{background:#1e2b43;color:#cbd5e1;border-radius:0}"
 "notebook{background:#0f1a2b;border:1px solid #263449;border-radius:10px}"
 "notebook header{background:#111c2e}notebook tab{padding:11px 18px;color:#94a3b8;font-weight:700}"
 "notebook tab:checked{background:#293b75;color:#fff}"
 "separator{background:#263449}"
 ".panel{background:#111c2e;border:1px solid #293a55;border-radius:12px;padding:8px}"
 ".login-logo{background:#263b78;color:#ffffff;font-size:25pt;font-weight:900;padding:13px 20px;border-radius:16px}"
 ".login-brand{font-size:21pt;font-weight:900;color:#f8fafc;letter-spacing:2px}"
 ".login-heading{font-size:19pt;font-weight:800;color:#f8fafc}"
 ".login-card{background:#111c2e;border:1px solid #293a55;border-radius:14px}"
 ".field-label{color:#aab9d0;font-size:9pt;font-weight:700}"
 "#primary-login{background:#405bd5;border:1px solid #6178ed;border-radius:9px;font-size:11pt;font-weight:800;padding:10px}"
 "#primary-login:hover{background:#526deb}"

 ".register-title{font-size:20pt;font-weight:900;color:#f8fafc}"
 ".register-subtitle{font-size:9pt;color:#94a3b8}"
 ".register-card{background:#111c2e;border:1px solid #293a55;border-radius:14px}"
 ".register-label{color:#aab9d0;font-size:9pt;font-weight:700}"
 ".register-note{color:#94a3b8;font-size:9pt}"
 ".register-primary{background:#405bd5;color:#fff;border:1px solid #6178ed;border-radius:8px;font-weight:800}"
 "combobox box{background:#111c2e;color:#f1f5f9}"
 "combobox button{background:#111c2e;color:#f1f5f9;border:1px solid #334155}"
 "combobox window{background:#111c2e;color:#f1f5f9}"
 "combobox window menu{background:#111c2e;color:#f1f5f9}"
 "combobox window menu menuitem{background:#111c2e;color:#e2e8f0;padding:8px}"
 "combobox window menu menuitem:hover{background:#3446a5;color:#fff}"
 "combobox menu{background:#111c2e;color:#f1f5f9}"
 "combobox menu menuitem{background:#111c2e;color:#e2e8f0;padding:8px}"
 "combobox menu menuitem:hover{background:#3446a5;color:#fff}"
;
 GtkCssProvider*p=gtk_css_provider_new();gtk_css_provider_load_from_data(p,css,-1,NULL);
 gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),GTK_STYLE_PROVIDER(p),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);g_object_unref(p);
}

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
 gtk_window_set_default_size(GTK_WINDOW(window),1050,720);
 gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
 g_signal_connect(window,"destroy",G_CALLBACK(on_window_destroy),NULL);

 GtkWidget*root=gtk_box_new(GTK_ORIENTATION_VERTICAL,12);
 workspace_root=root;
 gtk_container_set_border_width(GTK_CONTAINER(root),16);
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
 gtk_widget_set_vexpand(mainrow,FALSE);
 gtk_widget_set_size_request(mainrow,-1,420);

 GtkWidget*left=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);
 gtk_widget_set_size_request(left,350,-1);
 gtk_style_context_add_class(gtk_widget_get_style_context(left),"panel");
 gtk_container_set_border_width(GTK_CONTAINER(left),14);
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
 gtk_style_context_add_class(gtk_widget_get_style_context(right),"panel");
 gtk_container_set_border_width(GTK_CONTAINER(right),14);
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
 gtk_widget_set_size_request(bottom,-1,155);
 gtk_widget_set_vexpand(bottom,FALSE);
 GtkWidget*res=gtk_box_new(GTK_ORIENTATION_VERTICAL,6);
 GtkWidget*hist=gtk_box_new(GTK_ORIENTATION_VERTICAL,6);
 gtk_widget_set_hexpand(res,TRUE);
 gtk_style_context_add_class(gtk_widget_get_style_context(res),"panel");
 gtk_container_set_border_width(GTK_CONTAINER(res),12);
 gtk_widget_set_hexpand(hist,TRUE);
 gtk_style_context_add_class(gtk_widget_get_style_context(hist),"panel");
 gtk_container_set_border_width(GTK_CONTAINER(hist),12);
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
 gtk_widget_set_size_request(hs,-1,105);
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
