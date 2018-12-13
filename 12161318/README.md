# crawler-project3

Andrew McCarthy, UCID: amccarthy13, Student ID: 12161318

Use 'make' to create executables mcrawler1 and mcrawler2.  mcrawler1 uses the same cookie for each thread.  mcrawler2 uses a unique cookie for each thread.

Flags:

-n: specifies maximum number of threads allowed to be running at any one time.
-h: specifies first url to crawl
-p: specifies port to be used to contact servers
-f: specifies directory to download files to


Notes for proper use:

please specify the start url as such: http://eychtipi.cs.uchicago.edu/.
please specify the download directory as such: first/second/third.  including a '/' in the beginning will fail to create a download directory.
max threads must be set to a value greater than 0.
All flags are required

sometimes 1 or 2 files the program downloads will be corrupted.  This rarely happens (maybe 1 in every 30 executions) so I think it may be a rare server bug.
in my testing, the largest number of files I was able to retrieve was 90 (including index.html files and excluding 404's)


How it works:

The program first searches for all local links in a single thread while loading file download urls into a queue.  After that, the program downloads all the file links found
using multi threading (1 thread per download).  In mcrawler1, each thread uses the same cookie.  In mcrawler2, each thread uses a unique cookie.  If a 402 error is encountered,
the file url is loaded back into a queue and the cookie that was used for that download is deleted and a new one is retrieved.  In the multi cookie version, the program retrieves
'max threads' number of cookies and loads them into a queue to be used for the downloads.  Once a cookie is used successfully in a download, it is loaded back into the queue.
If a 402 error is encountered, the cookie that was used is deleted and a new one is loaded into the back of the queue.  In the single cookie version, the program retrieves one
cookie to be used in all threads.  If a 402 error is encountered, the cookie is replaced with a new one.

To implement multi threading, the program continually loops through the list of files to download and will start downloading in a new thread if the current number of active threads is less
than the max thread limit.  Once a download is complete, its thread is joined and the thread count is reduced by one.
If the current thread count is equal to the max thread limit, the loop will stop and wait for a thread to join before starting a new download in a new thread.
If the max thread limit is set to 1, the main thread will wait between downloads for the downloading thread to finish before starting a new one.  The program also makes use of locks to
prevent race conditions between threads.

The program is divided into 6 files.  The parser file and its associated header file contain all the functions used for parsing html output retrieved from the server.  The socket
file and its associated header file contain all functions used for connecting to the server and retrieving and storing links and download urls.  The mcrawler programs contain the main function
and all functions used for scheduling crawling and downloading.

'make clean' will deleted all .exe files and .o files as well as file types associated with the downloaded files


Issues:

In my program, all http responses, and by extension download urls, are converted to lower case.  All files I was able to retrieve are entirely lowercase except for 1.  This caused that file to 404 when it was actually
there.  I couldn't figure out in time how to keep my http responses in upper case without breaking the program, so I included a hacky fix where if the specific upper case file is found it is changed to
the correct name with upper case.

I wasn't able to find any other major issues with the final version of my code.  The execution time varies from about 20 seconds to about 1 minute.