bool move_media(char* old_location, char* ext, int log_fd);
void create_dir(int year, int month, char* friendly_dirname, char* friendly_subdir);

void get_file_rename(bool is_jpg, char* file_rename, char* subdir, struct tm *time, 
                        struct jpg_time *time_jpg);

void add_new_file(char* file_rename, char* dir, char* ext, char* old_location, 
                    char* file_name);

bool get_media_extension(char* name, char* ext);