1)COMMUNICATION

Each request creates a MirrorManager thread which activates a different connection to the specific ContentServer. Thread terminates and returns the address of the non-existent ContentServer, in case that the requested ContentServer does not exist. If the connection is successfull, the LIST request is sent to the ContentServer (ContentServerID= the whole request i.e. linux29.di.uoa.gr:9000:./folder:2). The ContentServer receives the message and it stores it in an array of structs requests. In this struct each ContentServerID is connected with its corresponding delay.

The list of the ContentServer's content is returned by the ContentServer itself via a file. For each file of this list:
    1)If the file/folder is "requested", is pushed in the buffer with the ContentServerID
    2)If the file/folder is "not requested", is ignored.
The worker is responsible to send (fetch function) the file/folder with its ContentServerID. The ContnentServer decides the delay of the reply(=the content of the requested file), depending on the ContentServerID. If the request is a folder, ContentServer returns "it's a directory". At last, worker stores the returned content locally.

2)SHARED MEMORY
Each time one thread is allowed to access the buffer(read/write). If the buffer is full, producers are suspended on condition variable prod_var. If the buffer is empty, consumers are suspended on condition variable cons_var. There is also one mutex_statistics that protects the shared variables for data about statistics. Those variables,
are modified by every worker (NumDevicesDone, filesTransferred, bytesTransferred, Average, Variance, NotWorking).

3)END
At first main program becomes suspended on condition variable AllDone. Then, statistics are printed and workers threads join. When NumDevicesDone==MirrorCounter-1 (every request has been served) main program wakes up to collect workers and to print the statistics. Meanwhile, the rest workers are informed by the variable endOfWorkers, that the program has come to an end, so they execute pthread_exit.

References:
Increasing calculating of mean and variance:http://people.ds.cam.ac.uk/fanf2/hermes/doc/antiforgery/stats.pdf
Sending a file through socket: https://stackoverflow.com/questions/2014033/send-and-receive-a-file-in-socket-programming-in-linux-with-c-c-gcc-g

Dimitra Mavroforaki: https://github.com/dimitramav

