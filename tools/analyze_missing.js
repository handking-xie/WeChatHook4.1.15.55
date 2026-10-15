const fs = require('fs');
const endpoints = JSON.parse(fs.readFileSync('d:\\WeChat-Hook\\tools\\showdoc_endpoints_summary.json', 'utf8'));

// Group endpoints by main functional areas
const categories = {};
for (const ep of endpoints) {
  const parts = ep.category.split(' > ');
  const mainCat = parts[0];
  if (!categories[mainCat]) categories[mainCat] = [];
  categories[mainCat].push(ep);
}

console.log('ShowDoc Functional Categories:');
for (const [k, v] of Object.entries(categories)) {
  console.log(`\n================== ${k} (${v.length} endpoints) ==================`);
  v.slice(0, 10).forEach(e => console.log(`  ${e.method} ${e.path} : ${e.name}`));
  if (v.length > 10) console.log(`  ... and ${v.length - 10} more`);
}
