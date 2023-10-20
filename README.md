## 1. Problem Description

In 2050, urban landscapes worldwide have transformed into high-tech smart schools. Every student in this campus has smart devices to interact in real-time with various services and information streams provided by the school. 

The school authorities would like to implement a system to facilitate better communication between students. To help the school, you are expected to implement a simplified multi-service bulletin board system: **csieBulletinBoard**.

There are some requests for csieBulletinBoard.

1. After a new client connects, the server **must** send current announcements automatically.
2. The bulletin board can have a maximum of `RECORD_NUM` announcements at a time. When there is only one server and the 11th announcement appears, it should directly overwrite the content of the 1st announcement, and so on.
3. A bulletin board server can support a maximum of `MAX_CLIENTS` clients connected simultaneously.
4. This bulletin board system can support a maximum of `5` servers simultaneously.



You are expected to complete the following tasks:

1. Implement the `server.c`/`client.c`. You can build them from scratch or from template we provided(but feel free to edit any part of code).
2. Modify the code in order that the `server` **will not be blocked** by any single request, but can deal with many requests simultaneously.
   ( Hint: implement multiplexing with `select()` or `poll()` and refrain from **busy waiting**. )
3. Handle every request is correctly.
   ( Hint: Use lock to avoid simultaneously writing same file.)

## 2. Compile & Run

The provided sample code can be compiled into sample server.

Feel free to modify any part of the code as you need, or implement your own server from scratch.

### Compile

You should write your own `Makefile` to compile your code.
Your `Makefile` may contain commands to generate `server` and `client`.

```Makefile
all:
    ## TODO：
    ## compile server.c and client.c to generate "server" and "client"
```

Also, your `Makefile` should be able to perform cleanup after the execution correctly (i.e, delete `server` and `client`).

```Makefile
clean:
    ## TODO：
    ## delete server and client
```

### Run

After you compile the code, you can connect to a running server and client with following command:

```bash
$ ./server {port}
```

```bash
$ ./client {ip} {port}
```

- There are at most `MAX_CLIENTS` clients connecting to server in the same time.


## 3. About the Record file

The servers will access the file `BulletinBoard`. The file contains up to `RECORD_NUM` records. There is a `BulletinBoard` file in your repository for testing your code. TAs will use file at `RECORD_PATH` while judging your homework, make sure your code does not depend on a fixed `BulletinBoard` file.

Following is the structure of a record defined in `hw1.h`:

```c
#define MAX_CLIENTS 20
#define FROM_LEN 5
#define CONTENT_LEN 20
#define RECORD_NUM 10
#define RECORD_PATH "./BulletinBoard"
typedef struct {
 char From[FROM_LEN];
 char Content[CONTENT_LEN];
} record;
```

Note that TA may use **another** header file during the test.

A record is considered empty and not a post if both of its attributes, `From` and `Content`, are empty string `""`.

- All `<user_input>` will contain only  `[a-zA-Z0-9_,.]`



## 4. Sample input and output

- All commands will input via `stdin` and only contents output to `stdout` will be graded.

### Server

#### Post

When client send a post request, server need to find the first unlocked record after `last` record in `BulletinBoard` and locked it immediately.

* `last` means the index of previous writing record on this server.
* `last` should be 0 if there is no previous post on this server.

After finishing the post, server should unlock the record and print following message:

```
[Log] Receive post from <FROM>
```

#### Pull

Server has to print a warning in following format if there's at least one locked post when server accepts a new connection or get a `pull` request.

```
[Warning] Try to access locked post - <number_of_locked_post>
```

Your server should respond to a `pull` request from client in **0.2s**, or you will fail to pass the testcase. 

### Client

Support 3 commands, `post`, `pull` and `exit`. All input are case sensitive and should end with `\n`(**LF not [CRLF](https://en.wikipedia.org/wiki/Newline)**).

The terminal should show following content once it connect to server.

```
==============================
Welcome to CSIE Bulletin board
==============================
FROM: TA
CONTENT:
All unlocked posts in BulletinBoard should be shown in this format ^-^
FROM: TA
CONTENT:
If the post is locked, you should skip it.
FROM: TA
CONTENT:
GLHF
==============================
```

Client process needs to print following message in a new line and wait for user's input after showing above content and each `post/pull` command.

```
Please enter your command (post/pull/exit): <user_input>
```

#### Post

Client need to send a post request to check if there is a unlocked post. If all ten posts in `BulletinBoard` are locked, **client** should print following error message and wait for next command. Note that client can only check if there is a unlocked post through server.

```
[Error] Maximum posting limit exceeded
```

If there is an writable post, wait for user's input at each `<user_input>` in following format and then send to server.

```
FROM: <user_input>
CONTENT:
<user_input>
```

#### Pull

Collect all unlocked posts from server and print in following format. If all `RECORD_NUM` posts are locked, still need to print two separated lines .

```
==============================
FROM: TA
CONTENT:
Print all unlocked posts here.
FROM: TA
CONTENT:
There should not be any extra spaces or lines.
==============================
```

#### Exit

End the client process.
