#include <stdio.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[])
{
    openlog(NULL, 0, LOG_USER);

    if (argc != 3)
    {
        //inform the user the incorrect args were passed. The %d returns argsc-1 to the log.
        syslog(LOG_ERR, "There was an incorrect number of arguments. Should have gotten: 2, received %d", argc-1);
        closelog();
    }
    
    const char *filepath = argv[1];
    const char *text = argv[2];

    // https://man7.org/linux/man-pages/man2/open.2.html, used as a resource to code open function
    // create a filepath if it doesn't exist, per flags. Could also use creat(), reading that open()
    // is more accepted;
    int fileDirectory = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 999);

    // on error -1 is returned, could implement errno to indicate error and bomb out of main if error happened.
    if (fileDirectory == -1)
    {
        syslog(LOG_ERR,"There was an error. Failed to open file: %s", filepath);
        closelog();
        return 1;
    }
    else
    {
        // https://man7.org/linux/man-pages/man2/write.2.html for write() function
        // write text (which is passed), until bytes are done being written
        ssize_t bytesToWrite = write(fileDirectory, text, strlen(text));
        if (bytesToWrite == -1)
        {
            syslog(LOG_ERR,"There was an error. Failed to write: \"%s\" to %s", text, filepath);
            closelog();
        }
        
        syslog(LOG_DEBUG, "Writing \"%s\" to \"%s\" where \"%s\" is the text string writen to file (second argument) and \"%s\" is the file created by the script", text, filepath, text, filepath);
        closelog();
        
        close(fileDirectory);
        return 0;
    }
}