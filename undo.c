bool undo_logged_changes(int undo_log_fd) {
    char line_buf_read[300];
    size_t res = 1;
    char *buf[1];
    int i = 0;
    while (res = read(undo_log_fd, buf, 1) == 1) {
        if (*buf != '\n') {
            line_buf_read[i++] = buf;
        } else {
            i = 0;
            char *return_dir = (char*) malloc(300);
            char *undo_dir = (char*) malloc(300);

            // first tok is a deliminator
            strtok(line_buf_read, "$");
            // second tok is the original directory
            return_dir = strtok(line_buf_read, "$");
            // third tok is the new directory
            undo_dir = strtok(line_buf_read, "$");
            // end of line should appear here.

            // rename will move the file back and restore the name
            rename(undo_dir, return_dir);            
            return true;
            
        }
    }
    return false;
}