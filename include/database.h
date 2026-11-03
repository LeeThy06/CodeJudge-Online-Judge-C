#ifndef OJ_DATABASE_H
#define OJ_DATABASE_H
#include <sqlite3.h>
#include <stddef.h>
typedef struct { int id; char title[160]; char description[1200]; char input_desc[500]; char output_desc[500]; char sample_in[500]; char sample_out[500]; int time_limit; } OJProblem;
typedef struct { int id; int problem_id; char input[2048]; char output[2048]; } OJTest;
typedef struct { int id; char username[80]; int problem_id; char problem_title[160]; char status[16]; int score; char detail[512]; char submitted_at[40]; } OJSubmission;
int db_open(const char *path);
void db_close(void);
sqlite3 *db_handle(void);
int db_auth(const char *username,const char *password,char *role,size_t role_cap);
int db_add_user(const char *username,const char *password,const char *display_name,const char *role);
int db_list_problems(OJProblem *items,int cap);
int db_get_problem(int id,OJProblem *p);
int db_add_problem(const OJProblem *p);
int db_update_problem(const OJProblem *p);
int db_delete_problem(int id);
int db_list_tests(int problem_id,OJTest *items,int cap);
int db_add_test(int problem_id,const char *input,const char *output);
int db_update_test(int test_id,const char *input,const char *output);
int db_delete_test(int test_id);
int db_save_submission(const char *username,int problem_id,const char *status,int score,const char *detail);
int db_update_submission(int submission_id,const char *status,int score,const char *detail);
int db_list_submissions(const char *username,OJSubmission *items,int cap);
int db_list_all_submissions(OJSubmission *items,int cap);
int db_leaderboard(char *out,size_t cap);
int db_count(const char *table);
int db_log(const char *message);
int db_list_logs(char *out,size_t cap);
#endif
