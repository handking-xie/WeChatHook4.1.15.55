const fs = require('fs');
const eps = JSON.parse(fs.readFileSync('d:\\WeChat-Hook\\tools\\showdoc_endpoints_summary.json', 'utf8'));

console.log('Search for Contact/Friend/Chatroom endpoints in ShowDoc:');
for (const ep of eps) {
  if (ep.name.includes('联系人') || ep.name.includes('通讯录') || ep.name.includes('群') || ep.name.includes('会话') || ep.path.toLowerCase().includes('contact') || ep.path.toLowerCase().includes('chatroom')) {
    console.log(`  ${ep.method} ${ep.path} -> ${ep.name} (${ep.category})`);
  }
}
