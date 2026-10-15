const fs = require('fs');
const path = require('path');

const needle = Buffer.from('https://u.wechat.com/');
const searchRoots = [
  'C:\\Users\\Admin\\AppData\\Roaming\\Tencent',
  'C:\\Users\\Admin\\xwechat_files',
  'C:\\Users\\Admin\\Documents\\WeChat Files'
];

let checkedFiles = 0;
let foundHits = [];

function searchDir(dir) {
  try {
    const entries = fs.readdirSync(dir, { withFileTypes: true });
    for (const ent of entries) {
      const fullPath = path.join(dir, ent.name);
      if (ent.isDirectory()) {
        searchDir(fullPath);
      } else if (ent.isFile()) {
        try {
          const stats = fs.statSync(fullPath);
          if (stats.size > 0 && stats.size < 50 * 1024 * 1024) {
            checkedFiles++;
            const buf = fs.readFileSync(fullPath);
            let idx = buf.indexOf(needle);
            if (idx !== -1) {
              const str = buf.slice(idx, idx + 80).toString('utf8').replace(/[^\x20-\x7E]/g, '');
              foundHits.push({ file: fullPath, text: str });
              console.log(`[FOUND] in ${fullPath}: ${str}`);
            }
          }
        } catch(e) {}
      }
    }
  } catch(e) {}
}

for (const root of searchRoots) {
  console.log('Searching root:', root);
  searchDir(root);
}

console.log(`Checked ${checkedFiles} files. Total found: ${foundHits.length}`);
