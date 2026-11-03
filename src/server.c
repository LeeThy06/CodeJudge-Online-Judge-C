#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#include "protocol.h"
#include "database.h"

#define MAX_CLIENTS 64
#define MAX_ROWS 1000
#define RUN_TIMEOUT 2
static GtkWidget *window,*status_label,*clients_label,*submissions_label,*problems_label,*log_view,*start_button,*stop_button;
static GtkWidget *problem_tree,*test_tree,*submission_tree,*leaderboard_view,*tests_problem_combo;
static GtkListStore *problem_store,*test_store,*submission_store;
static int listen_fd=-1,running=0,active_clients=0,total_submissions=0;
static pthread_t accept_thread;
static pthread_mutex_t state_mutex=PTHREAD_MUTEX_INITIALIZER;
static void ui_log(const char *msg);
static char *b64(const char*s){return g_base64_encode((const guchar*)(s?s:""),strlen(s?s:""));}
static char *unb64(const char*s){gsize n=0;guchar*p=g_base64_decode(s,&n);char*r=g_malloc(n+1);memcpy(r,p,n);r[n]=0;g_free(p);return r;}
static void send_line(int fd,const char*s){send_all(fd,s,strlen(s));}
static int wait_with_timeout(pid_t pid,int sec,int*st){time_t start=time(NULL);for(;;){pid_t r=waitpid(pid,st,WNOHANG);if(r==pid)return 0;if(r<0){if(errno==EINTR)continue;return -1;}if(time(NULL)-start>=sec){kill(pid,SIGKILL);waitpid(pid,st,0);return 1;}usleep(20000);}}
static int write_file(const char*p,const char*s){int fd=open(p,O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0)return -1;size_t n=strlen(s),o=0;while(o<n){ssize_t w=write(fd,s+o,n-o);if(w<0){if(errno==EINTR)continue;close(fd);return -1;}o+=(size_t)w;}close(fd);return 0;}
static void trim(char*s){size_t n=strlen(s);while(n&&(s[n-1]=='\n'||s[n-1]=='\r'||s[n-1]==' '||s[n-1]=='\t'))s[--n]=0;}
static int equal_files(const char*a,const char*b){FILE*x=fopen(a,"r"),*y=fopen(b,"r");if(!x||!y){if(x)fclose(x);if(y)fclose(y);return 0;}char p[4096],q[4096];int ok=1;for(;;){char*rx=fgets(p,sizeof p,x),*ry=fgets(q,sizeof q,y);if(!rx||!ry){ok=rx==NULL&&ry==NULL;break;}trim(p);trim(q);if(strcmp(p,q)){ok=0;break;}}fclose(x);fclose(y);return ok;}
static void judge(int pid,const char*code,char*status,int*score,char*detail,size_t dcap){char templ[]="/tmp/oj-XXXXXX";char*dir=mkdtemp(templ);*score=0;if(!dir){strcpy(status,"RE");snprintf(detail,dcap,"Cannot create judge workspace");return;}char src[PATH_MAX],bin[PATH_MAX],err[PATH_MAX];snprintf(src,sizeof src,"%s/main.c",dir);snprintf(bin,sizeof bin,"%s/main",dir);snprintf(err,sizeof err,"%s/compiler.err",dir);if(write_file(src,code)<0){strcpy(status,"RE");snprintf(detail,dcap,"Cannot write source");return;}pid_t c=fork();if(c==0){int e=open(err,O_WRONLY|O_CREAT|O_TRUNC,0600);if(e>=0){dup2(e,2);close(e);}execlp("gcc","gcc","-std=c11","-O2","-Wall","-Wextra",src,"-o",bin,(char*)NULL);_exit(127);}if(c<0){strcpy(status,"RE");snprintf(detail,dcap,"Cannot launch GCC");return;}int st=0,tm=wait_with_timeout(c,8,&st);if(tm==1){strcpy(status,"TLE");snprintf(detail,dcap,"Compilation timeout");return;}if(!WIFEXITED(st)||WEXITSTATUS(st)){
  strcpy(status,"CE");FILE*ef=fopen(err,"r");char diagnostic[380]={0};
  if(ef){size_t got=fread(diagnostic,1,sizeof diagnostic-1,ef);diagnostic[got]='\0';fclose(ef);}
  for(size_t k=0;diagnostic[k];k++)if(diagnostic[k]=='\n'||diagnostic[k]=='\r')diagnostic[k]=' ';
  if(diagnostic[0])snprintf(detail,dcap,"Compilation failed: %.430s",diagnostic);
  else snprintf(detail,dcap,"Compilation failed; check C syntax and compiler diagnostics");
  return;
 }
 OJTest tests[256];int n=db_list_tests(pid,tests,256),passed=0;if(n<=0){strcpy(status,"RE");snprintf(detail,dcap,"No test cases configured");return;}for(int i=0;i<n;i++){char in[PATH_MAX],out[PATH_MAX],exp[PATH_MAX];snprintf(in,sizeof in,"%s/in%d",dir,i);snprintf(out,sizeof out,"%s/out%d",dir,i);snprintf(exp,sizeof exp,"%s/exp%d",dir,i);write_file(in,tests[i].input);write_file(exp,tests[i].output);pid_t r=fork();if(r==0){int fi=open(in,O_RDONLY),fo=open(out,O_WRONLY|O_CREAT|O_TRUNC,0600),fe=open("/dev/null",O_WRONLY);if(fi>=0){dup2(fi,0);close(fi);}if(fo>=0){dup2(fo,1);close(fo);}if(fe>=0){dup2(fe,2);close(fe);}execl(bin,bin,(char*)NULL);_exit(127);}if(r<0)continue;tm=wait_with_timeout(r,2,&st);if(tm==1){strcpy(status,"TLE");*score=passed*10/n;snprintf(detail,dcap,"Time limit exceeded on test %d",i+1);return;}if(!WIFEXITED(st)||WEXITSTATUS(st)){strcpy(status,"RE");*score=passed*10/n;snprintf(detail,dcap,"Runtime error on test %d",i+1);return;}if(equal_files(out,exp))passed++;}
 *score=passed*10/n;strcpy(status,passed==n?"AC":"WA");snprintf(detail,dcap,"Passed %d/%d test cases",passed,n);
}
static void send_problem_rows(int fd){OJProblem a[256];int n=db_list_problems(a,256);char line[8192];for(int i=0;i<n;i++){char*t=b64(a[i].title),*d=b64(a[i].description),*in=b64(a[i].input_desc),*out=b64(a[i].output_desc),*si=b64(a[i].sample_in),*so=b64(a[i].sample_out);snprintf(line,sizeof line,"PROBLEM|%d|%s|%s|%s|%s|%s|%s|%d\n",a[i].id,t,d,in,out,si,so,a[i].time_limit);send_line(fd,line);g_free(t);g_free(d);g_free(in);g_free(out);g_free(si);g_free(so);}send_line(fd,"END\n");}
static void send_history(int fd,const char*u){OJSubmission a[200];int n=db_list_submissions(u,a,200);char line[2048];for(int i=0;i<n;i++){char*t=b64(a[i].problem_title),*d=b64(a[i].detail);snprintf(line,sizeof line,"HISTORY|%d|%d|%s|%s|%d|%s|%s\n",a[i].id,a[i].problem_id,t,a[i].status,a[i].score,d,a[i].submitted_at);send_line(fd,line);g_free(t);g_free(d);}send_line(fd,"END\n");}
static void *client_worker(void*arg){
 int fd=*(int*)arg;free(arg);pthread_mutex_lock(&state_mutex);active_clients++;pthread_mutex_unlock(&state_mutex);
 char line[OJ_MAX_LINE];
 while(recv_line(fd,line,sizeof line)>0){

  if(!strncmp(line,"REGISTER|",9)){
   char*save=NULL;
   strtok_r(line,"|",&save);
   char*u=strtok_r(NULL,"|",&save);
   char*pw=strtok_r(NULL,"|",&save);
   char*name=strtok_r(NULL,"|",&save);

   if(!u||!pw||!name||
      strlen(u)<3||strlen(u)>32||
      strlen(pw)<6||strlen(pw)>128||
      strlen(name)==0||strlen(name)>80){
    send_line(fd,"REGISTER|FAIL\n");
   }else{
    int added=db_add_user(u,pw,name,"STUDENT");
    if(added==1){
     send_line(fd,"REGISTER|OK\n");
     char msg[256];
     snprintf(msg,sizeof msg,"New student account registered: %s",u);
     db_log(msg);
     ui_log(msg);
    }else if(added==0){
     send_line(fd,"REGISTER|EXISTS\n");
    }else{
     send_line(fd,"REGISTER|FAIL\n");
    }
   }
  }else if(!strncmp(line,"AUTH|",5)){
   char*u=strtok(line+5,"|"),*pw=strtok(NULL,"|");char role[32];
   if(u&&pw&&db_auth(u,pw,role,sizeof role)){char*r=b64(role);char resp[128];snprintf(resp,sizeof resp,"AUTH|OK|%s\n",r);send_line(fd,resp);g_free(r);char msg[256];snprintf(msg,sizeof msg,"User %s logged in",u);db_log(msg);ui_log(msg);}
   else send_line(fd,"AUTH|FAIL|Invalid username or password\n");
  }else if(!strcmp(line,"LIST"))send_problem_rows(fd);
  else if(!strncmp(line,"HISTORY|",8))send_history(fd,line+8);
  else if(!strcmp(line,"LEADERBOARD")){char out[8192];db_leaderboard(out,sizeof out);send_all(fd,out,strlen(out));send_line(fd,"END\n");}
  else if(!strncmp(line,"SUBMIT|",7)){
   char*save=NULL;strtok_r(line,"|",&save);char*u=strtok_r(NULL,"|",&save),*ids=strtok_r(NULL,"|",&save),*sz=strtok_r(NULL,"|",&save);int id=ids?atoi(ids):0;unsigned long n=sz?strtoul(sz,NULL,10):0;
   if(u&&id>0&&n>0&&n<=OJ_MAX_CODE){char*code=calloc(n+1,1);if(code&&recv_all(fd,code,n)==0){char st[16],detail[512];int score=0;int sid=db_save_submission(u,id,"JUDGING",0,"Submission received; judging in progress");pthread_mutex_lock(&state_mutex);total_submissions++;pthread_mutex_unlock(&state_mutex);ui_log("Judging started");judge(id,code,st,&score,detail,sizeof detail);if(sid>0)db_update_submission(sid,st,score,detail);char resp[1024];snprintf(resp,sizeof resp,"RESULT|%s|%d|%s\n",st,score,detail);send_line(fd,resp);char msg[512];snprintf(msg,sizeof msg,"%s submitted Problem %d: %s (%d/10)",u,id,st,score);db_log(msg);ui_log(msg);}free(code);}else send_line(fd,"RESULT|RE|0|Invalid submission request\n");
  }else send_line(fd,"ERROR|Unknown command\n");
 }
 close(fd);pthread_mutex_lock(&state_mutex);active_clients--;pthread_mutex_unlock(&state_mutex);return NULL;
}
static void *accept_loop(void*x){(void)x;for(;;){pthread_mutex_lock(&state_mutex);int run=running,s=listen_fd;pthread_mutex_unlock(&state_mutex);if(!run||s<0)break;struct sockaddr_in a;socklen_t n=sizeof a;int fd=accept(s,(struct sockaddr*)&a,&n);if(fd<0){if(errno==EINTR)continue;pthread_mutex_lock(&state_mutex);run=running;pthread_mutex_unlock(&state_mutex);if(!run)break;continue;}char ip[64];inet_ntop(AF_INET,&a.sin_addr,ip,sizeof ip);char msg[128];snprintf(msg,sizeof msg,"Client connected: %s:%d",ip,ntohs(a.sin_port));db_log(msg);ui_log(msg);int*p=malloc(sizeof *p);if(!p){close(fd);continue;}*p=fd;pthread_t t;if(!pthread_create(&t,NULL,client_worker,p))pthread_detach(t);else{close(fd);free(p);}}return NULL;}
static gboolean log_ui_cb(gpointer data){char*m=data;GtkTextBuffer*b=gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_view));GtkTextIter e;gtk_text_buffer_get_end_iter(b,&e);gtk_text_buffer_insert(b,&e,m,-1);gtk_text_buffer_insert(b,&e,"\n",1);g_free(m);return G_SOURCE_REMOVE;}
static void ui_log(const char*m){char line[700];time_t t=time(NULL);struct tm tmv;localtime_r(&t,&tmv);char ts[32];strftime(ts,sizeof ts,"%H:%M:%S",&tmv);snprintf(line,sizeof line,"[%s] %s",ts,m);g_idle_add(log_ui_cb,g_strdup(line));}
static void refresh_problems(void){gtk_list_store_clear(problem_store);OJProblem a[256];int n=db_list_problems(a,256);for(int i=0;i<n;i++){GtkTreeIter it;gtk_list_store_append(problem_store,&it);gtk_list_store_set(problem_store,&it,0,a[i].id,1,a[i].title,2,a[i].description,-1);}gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(tests_problem_combo));for(int i=0;i<n;i++){char s[220];snprintf(s,sizeof s,"%d - %s",a[i].id,a[i].title);gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tests_problem_combo),s);}if(n)gtk_combo_box_set_active(GTK_COMBO_BOX(tests_problem_combo),0);}
static int selected_id(GtkWidget*tree){GtkTreeSelection*sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));GtkTreeModel*m;GtkTreeIter it;if(!gtk_tree_selection_get_selected(sel,&m,&it))return 0;int id=0;gtk_tree_model_get(m,&it,0,&id,-1);return id;}
static void refresh_tests(void){gtk_list_store_clear(test_store);int idx=gtk_combo_box_get_active(GTK_COMBO_BOX(tests_problem_combo));if(idx<0)return;char*s=gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(tests_problem_combo));if(!s)return;int pid=atoi(s);g_free(s);OJTest a[256];int n=db_list_tests(pid,a,256);for(int i=0;i<n;i++){GtkTreeIter it;gtk_list_store_append(test_store,&it);gtk_list_store_set(test_store,&it,0,a[i].id,1,a[i].input,2,a[i].output,-1);}}
static void refresh_submissions(void){gtk_list_store_clear(submission_store);OJSubmission a[500];int n=db_list_all_submissions(a,500);for(int i=0;i<n;i++){GtkTreeIter it;gtk_list_store_append(submission_store,&it);gtk_list_store_set(submission_store,&it,0,a[i].id,1,a[i].username,2,a[i].problem_title,3,a[i].status,4,a[i].score,5,a[i].submitted_at,-1);}}
static void refresh_leaderboard(void){char out[8192];db_leaderboard(out,sizeof out);gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(leaderboard_view)),out,-1);}
static GtkWidget*entry_row(GtkWidget*grid,const char*label,int row,int multi){GtkWidget*l=gtk_label_new(label);gtk_widget_set_halign(l,GTK_ALIGN_START);gtk_grid_attach(GTK_GRID(grid),l,0,row,1,1);GtkWidget*w;if(multi){w=gtk_text_view_new();gtk_widget_set_size_request(w,380,75);}else{w=gtk_entry_new();gtk_widget_set_size_request(w,380,-1);}gtk_grid_attach(GTK_GRID(grid),w,1,row,1,1);return w;}
static void get_widget_text(GtkWidget*w,char*out,size_t cap){if(GTK_IS_TEXT_VIEW(w)){GtkTextBuffer*b=gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));GtkTextIter a,z;gtk_text_buffer_get_bounds(b,&a,&z);char*s=gtk_text_buffer_get_text(b,&a,&z,FALSE);snprintf(out,cap,"%s",s);g_free(s);}else snprintf(out,cap,"%s",gtk_entry_get_text(GTK_ENTRY(w)));}
static void set_widget_text(GtkWidget*w,const char*s){if(GTK_IS_TEXT_VIEW(w))gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(w)),s,-1);else gtk_entry_set_text(GTK_ENTRY(w),s);}


static void problem_add_class(GtkWidget *w, const char *name) {
    gtk_style_context_add_class(
        gtk_widget_get_style_context(w), name);
}

static GtkWidget *problem_make_label(const char *text) {
    GtkWidget *l = gtk_label_new(text);
    gtk_widget_set_halign(l, GTK_ALIGN_START);
    problem_add_class(l, "problem-field-label");
    return l;
}

static GtkWidget *problem_make_entry(const char *placeholder) {
    GtkWidget *e = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(e), placeholder);
    gtk_widget_set_hexpand(e, TRUE);
    gtk_widget_set_size_request(e, -1, 40);
    problem_add_class(e, "problem-input");
    return e;
}

static GtkWidget *problem_make_editor(int height) {
    GtkWidget *view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(
        GTK_TEXT_VIEW(view), GTK_WRAP_WORD_CHAR);
    gtk_widget_set_hexpand(view, TRUE);
    gtk_widget_set_size_request(view, -1, height);
    problem_add_class(view, "problem-editor");

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    gtk_widget_set_hexpand(scroll, TRUE);
    problem_add_class(scroll, "problem-scroll");

    g_object_set_data(G_OBJECT(view), "problem-scroll", scroll);
    return view;
}

static GtkWidget *problem_make_field(
    const char *label, GtkWidget *input) {

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);
    gtk_box_pack_start(
        GTK_BOX(box), problem_make_label(label),
        FALSE, FALSE, 0);

    GtkWidget *widget = g_object_get_data(
        G_OBJECT(input), "problem-scroll");

    if (widget)
        gtk_box_pack_start(GTK_BOX(box), widget, TRUE, TRUE, 0);
    else
        gtk_box_pack_start(GTK_BOX(box), input, FALSE, FALSE, 0);

    return box;
}

static GtkWidget *problem_make_section(const char *title) {
    GtkWidget *section =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    problem_add_class(section, "problem-section");

    GtkWidget *heading = gtk_label_new(title);
    gtk_widget_set_halign(heading, GTK_ALIGN_START);
    problem_add_class(heading, "problem-section-title");

    gtk_box_pack_start(
        GTK_BOX(section), heading, FALSE, FALSE, 0);

    return section;
}

static GtkWidget *problem_make_pair(
    GtkWidget *left, GtkWidget *right) {

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_hexpand(row, TRUE);

    gtk_box_pack_start(GTK_BOX(row), left, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(row), right, TRUE, TRUE, 0);

    return row;
}

static void problem_dialog(int edit) {
    int id = edit ? selected_id(problem_tree) : 0;
    if (edit && !id) return;

    OJProblem old = {0};
    if (edit) db_get_problem(id, &old);

    GtkWidget *d = gtk_dialog_new_with_buttons(
        edit ? "Edit Problem" : "Create New Problem",
        GTK_WINDOW(window),
        GTK_DIALOG_MODAL,
        "Cancel", GTK_RESPONSE_CANCEL,
        edit ? "Save Changes" : "Create Problem",
        GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_window_set_default_size(GTK_WINDOW(d), 940, 800);
    gtk_window_set_resizable(GTK_WINDOW(d), TRUE);
    problem_add_class(d, "problem-dialog");

    GtkWidget *content =
        gtk_dialog_get_content_area(GTK_DIALOG(d));

    GtkWidget *body =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);

    gtk_container_set_border_width(GTK_CONTAINER(body), 20);
    gtk_box_pack_start(GTK_BOX(content), body, TRUE, TRUE, 0);

    /* Header */
    GtkWidget *header =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *heading = gtk_label_new(
        edit ? "EDIT PROBLEM" : "CREATE NEW PROBLEM");

    gtk_widget_set_halign(heading, GTK_ALIGN_START);
    problem_add_class(heading, "problem-heading");

    GtkWidget *subtitle = gtk_label_new(
        "Create and configure your programming challenge.");

    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    problem_add_class(subtitle, "problem-subtitle");

    gtk_box_pack_start(GTK_BOX(header), heading, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header), subtitle, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(body), header, FALSE, FALSE, 0);

    /* Basic information */
    GtkWidget *basic = problem_make_section(
        "01   BASIC INFORMATION");

    GtkWidget *title = problem_make_entry(
        "Enter problem title...");

    GtkWidget *desc = problem_make_editor(85);

    gtk_box_pack_start(
        GTK_BOX(basic),
        problem_make_field("PROBLEM TITLE", title),
        FALSE, FALSE, 0);

    gtk_box_pack_start(
        GTK_BOX(basic),
        problem_make_field("DESCRIPTION", desc),
        FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(body), basic, FALSE, FALSE, 0);

    /* Input and output */
    GtkWidget *io = problem_make_section(
        "02   INPUT / OUTPUT FORMAT");

    GtkWidget *inp = problem_make_editor(90);
    GtkWidget *out = problem_make_editor(90);

    gtk_box_pack_start(
        GTK_BOX(io),
        problem_make_pair(
            problem_make_field("INPUT FORMAT", inp),
            problem_make_field("OUTPUT FORMAT", out)),
        FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(body), io, FALSE, FALSE, 0);

    /* Sample tests */
    GtkWidget *samples = problem_make_section(
        "03   SAMPLE TEST CASE");

    GtkWidget *si = problem_make_editor(85);
    GtkWidget *so = problem_make_editor(85);

    gtk_box_pack_start(
        GTK_BOX(samples),
        problem_make_pair(
            problem_make_field("SAMPLE INPUT", si),
            problem_make_field("SAMPLE OUTPUT", so)),
        FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(body), samples, FALSE, FALSE, 0);

    /* Settings */
    GtkWidget *settings = problem_make_section(
        "04   EXECUTION SETTINGS");

    GtkWidget *time_row =
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

    GtkWidget *time_label =
        problem_make_label("TIME LIMIT");

    GtkWidget *tl = problem_make_entry("2");
    gtk_widget_set_size_request(tl, 130, 40);
    gtk_widget_set_hexpand(tl, FALSE);

    GtkWidget *seconds =
        gtk_label_new("seconds");

    problem_add_class(seconds, "problem-subtitle");

    gtk_box_pack_start(
        GTK_BOX(time_row), time_label, FALSE, FALSE, 0);
    gtk_box_pack_start(
        GTK_BOX(time_row), tl, FALSE, FALSE, 0);
    gtk_box_pack_start(
        GTK_BOX(time_row), seconds, FALSE, FALSE, 0);

    gtk_box_pack_start(
        GTK_BOX(settings), time_row, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(body), settings, FALSE, FALSE, 0);

    /* Restore existing data when editing */
    set_widget_text(title, old.title);
    set_widget_text(desc, old.description);
    set_widget_text(inp, old.input_desc);
    set_widget_text(out, old.output_desc);
    set_widget_text(si, old.sample_in);
    set_widget_text(so, old.sample_out);

    char time_value[16];
    snprintf(time_value, sizeof(time_value), "%d",
             old.time_limit ? old.time_limit : 2);
    set_widget_text(tl, time_value);

    gtk_dialog_set_default_response(
        GTK_DIALOG(d), GTK_RESPONSE_ACCEPT);

    gtk_widget_show_all(d);
    gtk_widget_grab_focus(title);

    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        OJProblem p = {0};
        p.id = id;

        get_widget_text(title, p.title, sizeof(p.title));
        get_widget_text(desc, p.description, sizeof(p.description));
        get_widget_text(inp, p.input_desc, sizeof(p.input_desc));
        get_widget_text(out, p.output_desc, sizeof(p.output_desc));
        get_widget_text(si, p.sample_in, sizeof(p.sample_in));
        get_widget_text(so, p.sample_out, sizeof(p.sample_out));

        char q[32];
        get_widget_text(tl, q, sizeof(q));
        p.time_limit = atoi(q);
        if (p.time_limit <= 0) p.time_limit = 2;

        if (strlen(p.title) && strlen(p.description)) {
            if (edit)
                db_update_problem(&p);
            else
                db_add_problem(&p);

            db_log(edit ? "Problem updated" : "Problem created");
            ui_log(edit ? "Problem updated" : "Problem created");
            refresh_problems();
        }
    }

    gtk_widget_destroy(d);
}


static void test_dialog(int edit) {
    int id = edit ? selected_id(test_tree) : 0;

    if (edit && !id) {
        GtkWidget *msg = gtk_message_dialog_new(
            GTK_WINDOW(window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "Please select a test case to edit."
        );
        gtk_dialog_run(GTK_DIALOG(msg));
        gtk_widget_destroy(msg);
        return;
    }

    int pid = 0;
    char problem_label[256] = "No problem selected";
    OJTest old = {0};

    if (edit) {
        /* Load the exact test case selected in the table */
        if (!db_get_test(id, &old)) {
            GtkWidget *msg = gtk_message_dialog_new(
                GTK_WINDOW(window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Cannot find the selected test case."
            );
            gtk_dialog_run(GTK_DIALOG(msg));
            gtk_widget_destroy(msg);
            return;
        }

        pid = old.problem_id;

        OJProblem problem = {0};
        if (db_get_problem(pid, &problem)) {
            g_snprintf(
                problem_label,
                sizeof(problem_label),
                "%d - %s",
                pid,
                problem.title
            );
        } else {
            g_snprintf(
                problem_label,
                sizeof(problem_label),
                "Problem ID: %d",
                pid
            );
        }
    } else {
        /* Add mode uses the currently selected problem */
        char *ps = gtk_combo_box_text_get_active_text(
            GTK_COMBO_BOX_TEXT(tests_problem_combo)
        );

        if (ps) {
            pid = atoi(ps);
            g_strlcpy(problem_label, ps, sizeof(problem_label));
            g_free(ps);
        }

        if (!pid) {
            GtkWidget *msg = gtk_message_dialog_new(
                GTK_WINDOW(window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_INFO,
                GTK_BUTTONS_OK,
                "Please select a problem first."
            );
            gtk_dialog_run(GTK_DIALOG(msg));
            gtk_widget_destroy(msg);
            return;
        }
    }

    GtkWidget *d = gtk_dialog_new_with_buttons(
        edit ? "Edit Test Case" : "Create Test Case",
        GTK_WINDOW(window),
        GTK_DIALOG_MODAL,
        "Cancel", GTK_RESPONSE_CANCEL,
        edit ? "Save Changes" : "Add Test Case",
        GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_window_set_default_size(GTK_WINDOW(d), 1000, 680);
    gtk_window_set_resizable(GTK_WINDOW(d), TRUE);
    gtk_window_set_position(GTK_WINDOW(d), GTK_WIN_POS_CENTER_ON_PARENT);
    problem_add_class(d, "test-dialog");

    GtkWidget *content =
        gtk_dialog_get_content_area(GTK_DIALOG(d));

    GtkWidget *body =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);

    gtk_container_set_border_width(GTK_CONTAINER(body), 24);
    gtk_box_pack_start(GTK_BOX(content), body, TRUE, TRUE, 0);

    /* Header */
    GtkWidget *header =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

    GtkWidget *eyebrow =
        gtk_label_new("CODEJUDGE  /  TEST CASE MANAGER");

    gtk_widget_set_halign(eyebrow, GTK_ALIGN_START);
    problem_add_class(eyebrow, "test-eyebrow");

    GtkWidget *heading =
        gtk_label_new(edit ? "Edit Test Case" : "Create Test Case");

    gtk_widget_set_halign(heading, GTK_ALIGN_START);
    problem_add_class(heading, "problem-heading");

    GtkWidget *subtitle =
        gtk_label_new(
            "Configure input data and the exact output expected from the solution.");

    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_label_set_line_wrap(GTK_LABEL(subtitle), TRUE);
    problem_add_class(subtitle, "problem-subtitle");

    gtk_box_pack_start(GTK_BOX(header), eyebrow, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header), heading, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header), subtitle, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(body), header, FALSE, FALSE, 0);

    /* Assigned problem card */
    GtkWidget *info =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    problem_add_class(info, "problem-section");

    GtkWidget *info_title =
        gtk_label_new("ASSIGNED PROBLEM");

    gtk_widget_set_halign(info_title, GTK_ALIGN_START);
    problem_add_class(info_title, "problem-section-title");

    GtkWidget *problem_name =
        gtk_label_new(problem_label);

    gtk_widget_set_halign(problem_name, GTK_ALIGN_START);
    gtk_label_set_selectable(GTK_LABEL(problem_name), TRUE);
    problem_add_class(problem_name, "test-problem-name");

    gtk_box_pack_start(GTK_BOX(info), info_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(info), problem_name, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(body), info, FALSE, FALSE, 0);

    /* Test data section */
    GtkWidget *section =
        problem_make_section("01  /  TEST DATA");

    GtkWidget *description =
        gtk_label_new("Provide the input and its corresponding expected output.");

    gtk_widget_set_halign(description, GTK_ALIGN_START);
    problem_add_class(description, "problem-subtitle");

    gtk_box_pack_start(
        GTK_BOX(section), description, FALSE, FALSE, 0);

    GtkWidget *input = problem_make_editor(250);
    GtkWidget *expected = problem_make_editor(250);

    GtkWidget *input_field =
        problem_make_field("STDIN  /  INPUT DATA", input);

    GtkWidget *output_field =
        problem_make_field("EXPECTED OUTPUT  /  STDOUT", expected);

    problem_add_class(input_field, "test-editor-card");
    problem_add_class(output_field, "test-editor-card");

    gtk_box_pack_start(
        GTK_BOX(section),
        problem_make_pair(input_field, output_field),
        TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(body), section, TRUE, TRUE, 0);

    /* Footer hint */
    GtkWidget *hint_box =
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    problem_add_class(hint_box, "test-hint");

    GtkWidget *hint_icon =
        gtk_label_new("i");

    problem_add_class(hint_icon, "test-hint-icon");

    GtkWidget *hint =
        gtk_label_new(
            "The judge compares your program output with the expected output. "
            "Pay attention to spaces and line breaks.");

    gtk_label_set_line_wrap(GTK_LABEL(hint), TRUE);
    gtk_widget_set_halign(hint, GTK_ALIGN_START);
    problem_add_class(hint, "problem-subtitle");

    gtk_box_pack_start(GTK_BOX(hint_box), hint_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hint_box), hint, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(body), hint_box, FALSE, FALSE, 0);

    /* Restore existing values when editing */
    set_widget_text(input, old.input);
    set_widget_text(expected, old.output);

    gtk_dialog_set_default_response(
        GTK_DIALOG(d), GTK_RESPONSE_ACCEPT);

    gtk_widget_show_all(d);
    gtk_widget_grab_focus(input);

    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        char a[2048] = {0};
        char b[2048] = {0};

        get_widget_text(input, a, sizeof(a));
        get_widget_text(expected, b, sizeof(b));

        if (strlen(a) || strlen(b)) {
            if (edit)
                db_update_test(id, a, b);
            else
                db_add_test(pid, a, b);

            db_log(edit ? "Test case updated" : "Test case added");
            ui_log(edit ? "Test case updated" : "Test case added");
            refresh_tests();
        }
    }

    gtk_widget_destroy(d);
}

static void on_add_problem(GtkButton*b,gpointer x){(void)b;(void)x;problem_dialog(0);}static void on_edit_problem(GtkButton*b,gpointer x){(void)b;(void)x;problem_dialog(1);}static void on_del_problem(GtkButton*b,gpointer x){(void)b;(void)x;int id=selected_id(problem_tree);if(!id)return;GtkWidget*d=gtk_message_dialog_new(GTK_WINDOW(window),GTK_DIALOG_MODAL,GTK_MESSAGE_WARNING,GTK_BUTTONS_YES_NO,"Delete Problem %d and its test cases?",id);if(gtk_dialog_run(GTK_DIALOG(d))==GTK_RESPONSE_YES){db_delete_problem(id);db_log("Problem deleted");refresh_problems();refresh_tests();}gtk_widget_destroy(d);}static void on_tests_problem_changed(GtkComboBox*b,gpointer x){(void)b;(void)x;refresh_tests();}
static void on_add_test(GtkButton*b,gpointer x){(void)b;(void)x;test_dialog(0);}static void on_edit_test(GtkButton*b,gpointer x){(void)b;(void)x;test_dialog(1);}static void on_del_test(GtkButton*b,gpointer x){(void)b;(void)x;int id=selected_id(test_tree);if(id){db_delete_test(id);db_log("Test case deleted");refresh_tests();}}
static void on_start(GtkButton*b,gpointer x){(void)b;(void)x;if(running)return;int s=socket(AF_INET,SOCK_STREAM,0);if(s<0){ui_log("Socket creation failed");return;}int opt=1;setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof opt);struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_addr.s_addr=INADDR_ANY;a.sin_port=htons(OJ_PORT);if(bind(s,(struct sockaddr*)&a,sizeof a)<0||listen(s,32)<0){ui_log(strerror(errno));close(s);return;}pthread_mutex_lock(&state_mutex);listen_fd=s;running=1;pthread_mutex_unlock(&state_mutex);pthread_create(&accept_thread,NULL,accept_loop,NULL);gtk_widget_set_sensitive(start_button,FALSE);gtk_widget_set_sensitive(stop_button,TRUE);gtk_label_set_text(GTK_LABEL(status_label),"ONLINE");db_log("Server started");ui_log("Server started on TCP port 5050");}
static void on_stop(GtkButton*b,gpointer x){(void)b;(void)x;pthread_mutex_lock(&state_mutex);int s=listen_fd,was=running;listen_fd=-1;running=0;pthread_mutex_unlock(&state_mutex);if(s>=0){shutdown(s,SHUT_RDWR);close(s);}if(was)pthread_join(accept_thread,NULL);gtk_widget_set_sensitive(start_button,TRUE);gtk_widget_set_sensitive(stop_button,FALSE);gtk_label_set_text(GTK_LABEL(status_label),"OFFLINE");db_log("Server stopped");ui_log("Server stopped");}
static gboolean refresh_stats(gpointer x){(void)x;refresh_submissions();refresh_leaderboard();char s[64];pthread_mutex_lock(&state_mutex);snprintf(s,sizeof s,"%d",active_clients);pthread_mutex_unlock(&state_mutex);gtk_label_set_text(GTK_LABEL(clients_label),s);snprintf(s,sizeof s,"%d",db_count("submissions"));gtk_label_set_text(GTK_LABEL(submissions_label),s);snprintf(s,sizeof s,"%d",db_count("problems"));gtk_label_set_text(GTK_LABEL(problems_label),s);return G_SOURCE_CONTINUE;}
static GtkWidget*stat_card(const char*t,GtkWidget**v){GtkWidget*b=gtk_box_new(GTK_ORIENTATION_VERTICAL,4);gtk_style_context_add_class(gtk_widget_get_style_context(b),"card");GtkWidget*l=gtk_label_new(t);gtk_widget_set_halign(l,GTK_ALIGN_START);*v=gtk_label_new("0");gtk_widget_set_halign(*v,GTK_ALIGN_START);gtk_style_context_add_class(gtk_widget_get_style_context(*v),"number");gtk_box_pack_start(GTK_BOX(b),l,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(b),*v,FALSE,FALSE,0);return b;}
static GtkWidget*tree_new(GtkListStore**store,const char*headers[],GType types[],int n){
 *store=gtk_list_store_newv(n,types);
 GtkWidget*t=gtk_tree_view_new_with_model(GTK_TREE_MODEL(*store));
 gtk_style_context_add_class(gtk_widget_get_style_context(t),"dark-table");

 GdkRGBA bg,fg,selected;
 gdk_rgba_parse(&bg,"#111c2e");
 gdk_rgba_parse(&fg,"#e2e8f0");
 gdk_rgba_parse(&selected,"#3446a5");

 gtk_widget_override_background_color(t,GTK_STATE_FLAG_NORMAL,&bg);

 for(int i=0;i<n;i++){
  GtkCellRenderer*r=gtk_cell_renderer_text_new();

  g_object_set(r,
   "foreground-rgba",&fg,
   "foreground-set",TRUE,
   "background-rgba",&bg,
   "background-set",TRUE,
   NULL);

  GtkTreeViewColumn*c=gtk_tree_view_column_new_with_attributes(
   headers[i],r,"text",i,NULL);

  gtk_tree_view_column_set_resizable(c,TRUE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(t),c);
 }

 gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(t),TRUE);
 gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(t),GTK_TREE_VIEW_GRID_LINES_HORIZONTAL);
 gtk_tree_view_set_enable_search(GTK_TREE_VIEW(t),TRUE);

 return t;
}
static GtkWidget*scroll_for(GtkWidget*w){GtkWidget*s=gtk_scrolled_window_new(NULL,NULL);gtk_container_add(GTK_CONTAINER(s),w);gtk_widget_set_vexpand(s,TRUE);gtk_widget_set_hexpand(s,TRUE);return s;}
static void load_css(void){

 const char*css=
 "window{background:#eaf6ff;color:#243447}"
 "label{color:#243447;font-size:10pt}"
 ".hero{font-size:27px;font-weight:800;color:#1769aa}"
 ".muted{color:#71869a;font-size:9pt}"
 ".card{background:#ffffff;border:1px solid #d5e9f8;border-radius:12px;padding:17px}"
 ".number{font-size:27px;font-weight:800;color:#2785d8}"
 "button{background:#2785d8;color:#ffffff;border:1px solid #2785d8;border-radius:8px;padding:9px 15px;font-weight:700}"
 "button:hover{background:#176db8}"
 "button:disabled{background:#dceaf4;color:#91a4b5;border-color:#dceaf4}"
 "entry,combobox{background:#ffffff;color:#243447;border:1px solid #c5dced;border-radius:7px;padding:7px}"
 "textview{background:#ffffff;color:#243447;font-family:monospace;padding:10px}"
 "treeview{background:#ffffff;color:#243447}"
 "treeview.view{background-color:#ffffff;color:#243447}"
 "treeview.view:selected{background-color:#b9e0ff;color:#174d78}"
 "treeview.view:backdrop{background-color:#ffffff;color:#243447}"
 "treeview.view:selected:backdrop{background-color:#d2ebff;color:#174d78}"
 "treeview.view header button{background:#dff1ff;color:#245577;border-radius:0}"
 "treeview.view header button:hover{background:#c6e6ff;color:#174d78}"
 "treeview.view row{background-color:#ffffff;color:#243447}"
 "treeview.view row:nth-child(even){background-color:#f0f8ff;color:#243447}"
 "treeview.view row:selected{background-color:#b9e0ff;color:#174d78}"
 "treeview.view cell{background-color:transparent;color:#243447}"
 "treeview.view cell:selected{background-color:#b9e0ff;color:#174d78}"
 "treeview.view:selected text{color:#174d78}"
 "treeview header button{background:#dff1ff;color:#245577;border-radius:0}"
 "treeview.dark-table,treeview.dark-table.view,treeview.dark-table row,treeview.dark-table cell{background-color:#ffffff;color:#243447}"
 "treeview.dark-table row:nth-child(even){background-color:#f0f8ff;color:#243447}"
 "treeview.dark-table row:selected,treeview.dark-table cell:selected{background-color:#b9e0ff;color:#174d78}"
 "scrolledwindow,scrolledwindow viewport,viewport{background-color:#ffffff;color:#243447}"
 "scrolledwindow{border:1px solid #d5e9f8;border-radius:9px}"
 "notebook stack,notebook > stack,notebook > stack > widget{background-color:#eaf6ff;color:#243447}"
 "window.popup,window.popup treeview,window.popup treeview.view,window.popup treeview row{background-color:#ffffff;color:#243447}"
 "window.popup treeview.view:selected,window.popup treeview row:selected{background-color:#b9e0ff;color:#174d78}"
 ".form-panel{background:#ffffff;border:1px solid #d5e9f8;border-radius:12px;padding:16px}"
 ".form-heading{font-size:19px;font-weight:800;color:#1769aa}"
 ".form-subtitle{color:#71869a;font-size:9pt}"
 ".form-label{color:#36566f;font-weight:700}"
 "entry.form-entry{background:#f8fcff;color:#243447;border:1px solid #c5dced;border-radius:7px;padding:9px}"
 "textview.form-text{background:#ffffff;color:#243447;font-family:monospace;padding:8px}"
 "scrolledwindow.form-scroll{background:#ffffff;border:1px solid #c5dced;border-radius:7px}"
 "dialog.problem-dialog,dialog.problem-dialog box{background-color:#eaf6ff;color:#243447}"
 ".problem-heading{font-size:22px;font-weight:800;color:#1769aa}"
 ".problem-subtitle{font-size:9pt;color:#71869a}"
 "dialog.test-dialog,dialog.test-dialog box{background-color:#eaf6ff;color:#243447}"
 ".test-eyebrow{font-size:9pt;font-weight:800;color:#2785d8;letter-spacing:1px}"
 ".test-problem-name{font-size:13pt;font-weight:700;color:#1769aa}"
 ".test-editor-card{background-color:#ffffff;border:1px solid #d5e9f8;border-radius:10px;padding:10px}"
 ".test-editor-card .problem-field-label{color:#2785d8;font-size:9pt;font-weight:800}"
 ".test-editor-card scrolledwindow.problem-scroll{background-color:#f8fcff;border:1px solid #c5dced;border-radius:8px}"
 ".test-editor-card textview.problem-editor{background-color:#ffffff;color:#243447;font-family:monospace;font-size:10pt;padding:12px}"
 ".test-hint{background-color:#e4f3ff;border:1px solid #c5e5fa;border-radius:9px;padding:12px}"
 ".test-hint-icon{background-color:#2785d8;color:#ffffff;border-radius:20px;font-weight:800;min-width:22px;min-height:22px}"
 "dialog.test-dialog button{min-height:22px;padding:9px 18px}"
 ".problem-section{background-color:#ffffff;border:1px solid #d5e9f8;border-radius:11px;padding:15px}"
 ".problem-section-title{font-size:11pt;font-weight:800;color:#2785d8}"
 ".problem-field-label{font-size:9pt;font-weight:700;color:#36566f}"
 "entry.problem-input{background-color:#f8fcff;color:#243447;border:1px solid #c5dced;border-radius:7px;padding:9px}"
 "textview.problem-editor{background-color:#ffffff;color:#243447;font-family:monospace;padding:8px}"
 "scrolledwindow.problem-scroll{background-color:#ffffff;border:1px solid #c5dced;border-radius:7px}"
 "notebook{background:#eaf6ff;border:1px solid #d5e9f8;border-radius:10px}"
 "notebook header{background:#e1f1fc}notebook tab{padding:12px 18px;color:#54738c;font-weight:700}"
 "notebook tab:checked{background:#2785d8;color:#ffffff}";
 GtkCssProvider*p=gtk_css_provider_new();gtk_css_provider_load_from_data(p,css,-1,NULL);
 gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),GTK_STYLE_PROVIDER(p),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);g_object_unref(p);
}
static void on_destroy(GtkWidget*w,gpointer x){(void)w;(void)x;on_stop(NULL,NULL);gtk_main_quit();}
int main(int argc,char**argv){signal(SIGPIPE,SIG_IGN);gtk_init(&argc,&argv);if(db_open("online_judge.db")<0){fprintf(stderr,"Cannot open online_judge.db\n");return 1;}load_css();window=gtk_window_new(GTK_WINDOW_TOPLEVEL);gtk_window_set_title(GTK_WINDOW(window),"CodeJudge | Administration Console");gtk_window_set_default_size(GTK_WINDOW(window),1120,760);g_signal_connect(window,"destroy",G_CALLBACK(on_destroy),NULL);GtkWidget*root=gtk_box_new(GTK_ORIENTATION_VERTICAL,12);gtk_container_set_border_width(GTK_CONTAINER(root),18);gtk_container_add(GTK_CONTAINER(window),root);GtkWidget*head=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);GtkWidget*brand=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);GtkWidget*t=gtk_label_new("CODEJUDGE  /  ADMIN CONTROL CENTER");gtk_widget_set_halign(t,GTK_ALIGN_START);gtk_style_context_add_class(gtk_widget_get_style_context(t),"hero");GtkWidget*sub=gtk_label_new("System overview  •  TCP Socket  •  SQLite  •  Automated C evaluation");gtk_widget_set_halign(sub,GTK_ALIGN_START);gtk_style_context_add_class(gtk_widget_get_style_context(sub),"muted");gtk_box_pack_start(GTK_BOX(brand),t,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(brand),sub,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(head),brand,TRUE,TRUE,0);gtk_box_pack_start(GTK_BOX(root),head,FALSE,FALSE,0);
GtkWidget*cards=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);GtkWidget*c1=stat_card("SERVER STATUS",&status_label),*c2=stat_card("CONNECTED CLIENTS",&clients_label),*c3=stat_card("TOTAL SUBMISSIONS",&submissions_label),*c4=stat_card("PROBLEMS",&problems_label);gtk_label_set_text(GTK_LABEL(status_label),"OFFLINE");gtk_box_pack_start(GTK_BOX(cards),c1,TRUE,TRUE,0);gtk_box_pack_start(GTK_BOX(cards),c2,TRUE,TRUE,0);gtk_box_pack_start(GTK_BOX(cards),c3,TRUE,TRUE,0);gtk_box_pack_start(GTK_BOX(cards),c4,TRUE,TRUE,0);gtk_box_pack_start(GTK_BOX(root),cards,FALSE,FALSE,0);GtkWidget*actions=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);start_button=gtk_button_new_with_label("▶ Start Server");stop_button=gtk_button_new_with_label("■ Stop Server");gtk_widget_set_sensitive(stop_button,FALSE);g_signal_connect(start_button,"clicked",G_CALLBACK(on_start),NULL);g_signal_connect(stop_button,"clicked",G_CALLBACK(on_stop),NULL);gtk_box_pack_start(GTK_BOX(actions),start_button,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(actions),stop_button,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(root),actions,FALSE,FALSE,0);
GtkWidget*nb=gtk_notebook_new();gtk_box_pack_start(GTK_BOX(root),nb,TRUE,TRUE,0);
GtkWidget*dashboard=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);log_view=gtk_text_view_new();gtk_text_view_set_editable(GTK_TEXT_VIEW(log_view),FALSE);gtk_box_pack_start(GTK_BOX(dashboard),gtk_label_new("RECENT ACTIVITY"),FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(dashboard),scroll_for(log_view),TRUE,TRUE,0);gtk_notebook_append_page(GTK_NOTEBOOK(nb),dashboard,gtk_label_new("Dashboard"));
const char*ph[]={"ID","Title","Description"};GType pt[]={G_TYPE_INT,G_TYPE_STRING,G_TYPE_STRING};problem_tree=tree_new(&problem_store,ph,pt,3);GtkWidget*pp=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);gtk_box_pack_start(GTK_BOX(pp),scroll_for(problem_tree),TRUE,TRUE,0);GtkWidget*pb=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);GtkWidget*add=gtk_button_new_with_label("+ Add Problem"),*edit=gtk_button_new_with_label("Edit Selected"),*del=gtk_button_new_with_label("Delete Selected");g_signal_connect(add,"clicked",G_CALLBACK(on_add_problem),NULL);g_signal_connect(edit,"clicked",G_CALLBACK(on_edit_problem),NULL);g_signal_connect(del,"clicked",G_CALLBACK(on_del_problem),NULL);gtk_box_pack_start(GTK_BOX(pb),add,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(pb),edit,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(pb),del,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(pp),pb,FALSE,FALSE,0);gtk_notebook_append_page(GTK_NOTEBOOK(nb),pp,gtk_label_new("Problems"));
const char*th[]={"Test ID","Input","Expected Output"};GType tt[]={G_TYPE_INT,G_TYPE_STRING,G_TYPE_STRING};test_tree=tree_new(&test_store,th,tt,3);GtkWidget*tp=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);tests_problem_combo=gtk_combo_box_text_new();g_signal_connect(tests_problem_combo,"changed",G_CALLBACK(on_tests_problem_changed),NULL);gtk_box_pack_start(GTK_BOX(tp),tests_problem_combo,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(tp),scroll_for(test_tree),TRUE,TRUE,0);GtkWidget*tb=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);GtkWidget*ta=gtk_button_new_with_label("+ Add Test Case"),*te=gtk_button_new_with_label("Edit Selected Test"),*td=gtk_button_new_with_label("Delete Selected Test");g_signal_connect(ta,"clicked",G_CALLBACK(on_add_test),NULL);g_signal_connect(te,"clicked",G_CALLBACK(on_edit_test),NULL);g_signal_connect(td,"clicked",G_CALLBACK(on_del_test),NULL);gtk_box_pack_start(GTK_BOX(tb),ta,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(tb),te,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(tb),td,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(tp),tb,FALSE,FALSE,0);gtk_notebook_append_page(GTK_NOTEBOOK(nb),tp,gtk_label_new("Test Cases"));
const char*sh[]={"ID","Student","Problem","Verdict","Score","Submitted At"};GType st[]={G_TYPE_INT,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_INT,G_TYPE_STRING};submission_tree=tree_new(&submission_store,sh,st,6);gtk_notebook_append_page(GTK_NOTEBOOK(nb),scroll_for(submission_tree),gtk_label_new("Submissions & Scores"));leaderboard_view=gtk_text_view_new();gtk_text_view_set_editable(GTK_TEXT_VIEW(leaderboard_view),FALSE);gtk_notebook_append_page(GTK_NOTEBOOK(nb),scroll_for(leaderboard_view),gtk_label_new("Leaderboard"));refresh_problems();refresh_tests();refresh_submissions();refresh_leaderboard();g_timeout_add_seconds(2,refresh_stats,NULL);char persisted[16000];if(db_list_logs(persisted,sizeof persisted)>0)gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_view)),persisted,-1);gtk_widget_show_all(window);ui_log("Admin console ready. Default account: admin (change credentials before real deployment).");gtk_main();db_close();return 0;}
