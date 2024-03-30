# node-mta
Mail Transfer Agent (email sender) addon for NodeJs

With this NodeJs addon, you can send email messages anywhere on the internet. 
No need to host a fully featured SMTP server. 
Tested with Gmail,Yahoo, Outlook principally. 

**Supported features**

* Send html or txt email with attachments
* [STARTTLS][1] command is always enabled
* Sign your emails with an RSA key and add the [DKIM-Signature][2] header
* Send mail to multiple email addresses. It will send all emails in a batch for each domain.

**Not supported**

* HTML template engine email 

* This will not receive emails, it's not an SMTP server


**How to use it ?**

* Include the file path of the root directory in the `dependencies` block of your `package.json`. You can change the C/C++ compiler 
in `binding.gyp`. You will find an example in `test.js` and you will need an RSA key pair
in */res/rsa\[.](public|private)$/*
 
Note: By default, this will be built with the debug flag.

* As a regular C/C++ code, but OpenSSL is a required dependency, it's used
for signing and hashing. NodeJs already include its own version of OpenSSL. 
You will have to make the DNS MX records request manually because I did it from the javascript part or NodeJs. You should also implement threading. Using -pthread is a bad idea with NodeJs addons because it may break its Event-Driven nature. 

All code related to NodeJs can be found in ./res/cpp/main.(hpp|cpp)$


**Please, don't spam people with this 🤭**


[1]: https://en.wikipedia.org/wiki/Opportunistic_TLS
[2]: https://en.wikipedia.org/wiki/DomainKeys_Identified_Mail
[3]: https://nodejs.org/api/addons.html
