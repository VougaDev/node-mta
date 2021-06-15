const Mod = require('./index');

(()=>{
  Mod.configure({
    hostname: "your-domain.com",
    privateKey: fs.readFileSync(`${__dirname}/res/rsa.private`, { encoding: "utf-8" }),
    publicKey: fs.readFileSync(`${__dirname}/res/rsa.public`, { encoding: "utf-8" }),
    port: 25,
    dkimSelector: "your-domain",
  });
  const email = {

    from: {
      name: "Your domain News",
      address: "news@your-domain.com"
    },
    //list: { name: "Your domain Users", address: "list@yourdomain.com" },
    to: [
      { name: "James Bond", address: "james@bond.com" }, 
      { name: "Simon Timplar", address: "simon@timplar.com" }
    ],
    subject: "Welcome -  We are happy to see you?",
    html: `
    <!Doctype html>
    <html>
      <head>
      <meta charset="utf-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      </head>
     <body>
     <strong>Hello Echo,</strong> <br><br>
     We constate that you're now ready to use your new tools. So now you can split it any way.
     <div>
    <h2>What is Lorem Ipsum?</h2>
    <p><strong>Lorem Ipsum</strong> is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the
     1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but
      also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset 
      sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of 
      Lorem Ipsum.</p>
      <a href="https://your-domain.com"> <img alt="your-best-bet.png" style="border:1px solid #131803; padding: 5px; border-radius:5px; width: 75px; height: 75px; margin: 0 auto;" 
      src="cid:logo.1235@your-domain.com"></a>
    </div>
     </body>
    <html>
    `,

    files: [
      {
        path: `${__dirname}/res/home.png`,
        inline: true,
        name: "image.png",
        id: "logo.1235"
      },
      {
        path: `${__dirname}/res/home.png`,
        inline: false,
        name: "image2.png",
        id: "logo.123"
      }

    ]
  };
  Mod.send(email);
})();