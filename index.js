let debug = false;
let addon = undefined;
try {
  addon = require('./build/Release/node_mta');
} catch (e) {
  debug = true;
  addon = require('./build/Debug/node_mta');
  console.warn(" -------------- Loading in debug mode --------------");
}
const dns = require('dns');
const fs = require('fs');
const Mod = {};
module.exports = Mod;

/**
 * 
 * @param {String} hostname The host ase used to send emails. can != tthe gost name in from email du to dkim relaxed
 * @param {String} privateKey Private key in PEM format
 * @param {String} publicKey Public key in PEM format
 * @param {Number} port the destination port to use for sending email to the remote host
 * @param {String} dkimSelector the dkim selector of the hostname
 */
Mod.configure = ({ hostname, privateKey, port, dkimSelector }) => {
  return addon.configure({ hostname, privateKey, port, dkimSelector });
};


/**
 * @param {Object} email
 * {
 *   from: {name,address},
 *   list: {name,address},
 *   to: { name, address} or [{ name, address}],
 *   replyTo (address),
 *   returnPath (address),
 *   subject,
 *   html,
 *   text,
 *   files: [
 *     {
 *       path,
 *       inline,
 *       name,
 *       id
 *     }
 *   ]
 * } 
 */
Mod.send = async email => {
  return new Promise((resolve, reject) => {
    let parts = undefined;
    if (Array.isArray(email.to)) parts = email.to[0].address.split("@");
    else parts = email.to.address.split("@");
    if (parts.length !== 2) return Promise.reject(new Error("MalformedEmailException --> " + email.to));
    const receiverHost = parts[parts.length - 1];
    dns.resolveMx(receiverHost, (error, addresses) => {
      if (error) {
        return reject(error);
      }
      if (email.body) {
        email.body = email.body.replace(/([ \t]{2,})/g, " ")
          .replace(/[\r\n]/g, "\r\n");
      }
      addresses = addresses.sort((a, b) => {
        /** @see https://stackoverflow.com/questions/3616505/sending-mail-with-smtp-to-multiple-addresses-relaying-the-message-to-different */
        return a.priority - b.priority
      });
      if (addresses && addresses.length > 0) {
        const sent = addon.send(email, addresses);
        if (sent) {
          return resolve();
        }
        return reject(new Error(`Failed to send email to ${email.to} - Something went wrong in native code`));
      }
      return reject(new Error("No MX Record found for receiver hostname ==> " + receiverHost));
    });
  });
};


