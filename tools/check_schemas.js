const fs = require('fs');
const eps = JSON.parse(fs.readFileSync('d:\\WeChat-Hook\\tools\\showdoc_endpoints_summary.json', 'utf8'));

const targets = [
  '/IsLoginStatus',
  '/GetContact',
  '/BatchGetContactBriefInfo',
  '/GetHeadIMG',
  '/GetContactLabelList',
  '/QueryChatRoomOwnerWxid',
  '/GetFriendTimeline',
  '/mmsnsuserpage'
];

for (const t of targets) {
  const matches = eps.filter(e => e.path === t);
  console.log(`\n========================================`);
  console.log(`Path: ${t} (${matches.length} matches)`);
  for (const m of matches) {
    console.log(`Name: ${m.name}`);
    console.log(`Category: ${m.category}`);
    console.log(`Body Example:\n${m.bodyExample}`);
    console.log(`Description:\n${m.description.slice(0, 300)}...`);
  }
}
