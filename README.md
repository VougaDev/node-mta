# node-mta
Mail Transfert Agent (email sender) addon for NodeJs

With this NodeJs addon, you can send email messages anywhere on the internet 
Without needing to host a fully featured SMTP server. 
Tested with Gmail,Yahoo, Outlook principally. 

**Supported features**

* Send html or txt email with attachments
* [STARTTLS][1] command, in fact this will not allow you to use clear text traffic
* Sign your emails with an RSA key and add the [DKIM-Signature][2] header
* Send mail to multiple emails. It will send all emails in a batch for each domain.

**Not supported**

* HTML template engine email - You can start implementing this, I will help 
you if you start. 

* This will not receive emails, it's not an SMTP server

* May not work on Windows, but I think I used only POSIX APIs.

**How to use it ?**

* Use it as an [NodeJs addon][3], include the file path to the root directory 
in the dependencies block of your package.json. You can change the C/C++ compiler 
in binding.gyp file. You will find an example in test.js and you will need an RSA key pair
in /res/rsa\[.](public|private)$/
 
Note: By default, this will be built with the debug flag.

* As a regular C/C++ code, but OpenSSL is a required dependency, it's used
for signing and hashing, by default NodeJs already include its own version of OpenSSL. 
You will also have to make the DNS MX records request by yourself because I did it from 
the javascript part or NodeJs. You should also implement threading. Using -pthread is a bad idea 
with NodeJs addons because it may break its Event-Driven nature. 

All code related to NodeJs can be found in ./res/cpp/main.(hpp|cpp)$

**How to contribute**

My main goal was not to publish this anywhere but only to use it in my own projects, because I did 
not want to call external APIs and pay money every month for that task.

I finally made it a module and shared it here without testing if it works but it should.
If you are interrested in making a pull requests I'm OK, particularly for security related patches.

I hope you will like using it. 
Please don't spam people with this. 


[1]: https://en.wikipedia.org/wiki/Opportunistic_TLS
[2]: https://en.wikipedia.org/wiki/DomainKeys_Identified_Mail
[3]: https://nodejs.org/api/addons.html
