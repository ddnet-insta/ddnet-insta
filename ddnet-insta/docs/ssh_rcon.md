# ddnet-insta rcon over ssh

In ddnet-insta the admin remote console (known as rcon console) can be reached via ssh.
Similar to the econ console in teeworlds or fifo in ddnet.


While encryption is working it is not its main selling point. This built in ssh server
is optimized for live user experience and scripting. Because the console only sees the
output of the command and not all logs like econ or no logs at all like fifo. It is
easy to follow along in live session and clear to parse command outputs in scripts.

## features

- password and ssh key login support
- command auto completion and help text preview
- fuzzy history search
- utf-8 input handling
- terminal like word jumping and cutting and pasting

![help text](https://raw.githubusercontent.com/ddnet-insta/images/f313edbb44306e7e81a1a5e68a93b76e8e89bdb3/ssh_rcon_helptext.png)
![history](https://raw.githubusercontent.com/ddnet-insta/images/f313edbb44306e7e81a1a5e68a93b76e8e89bdb3/ssh_rcon_history.png)
![preview](https://raw.githubusercontent.com/ddnet-insta/images/f313edbb44306e7e81a1a5e68a93b76e8e89bdb3/ssh_rcon_preview.png)

## setup

You need to compile the server with the cmake flag `-DSSH=ON` and need libssh installed.
And then add the following to your autoexec_server.cfg

```
sv_ssh 1
sv_ssh_port 2222
sv_ssh_password_authentication 1
```

Then after you started the server you can connect to it like this

```
ssh root@localhost -p 2222
```

The password will be your `sv_rcon_password`


Alternatively you can also add your ssh key to the authorized_keys file which is located
in your ddnet storage location under ssh/authorized_keys. So for example `~/.teeworlds/ssh/authorized_keys` or wherever that is on your system.
