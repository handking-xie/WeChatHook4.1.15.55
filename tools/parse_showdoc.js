const fs = require('fs');

const raw = fs.readFileSync('d:\\WeChat-Hook\\PCWeixinHook_ShowDoc_All_Apifox.json', 'utf8');
const data = JSON.parse(raw);

const allEndpoints = [];

function traverse(items, currentCategory) {
  for (const item of items) {
    if (item.item && Array.isArray(item.item)) {
      const catName = currentCategory ? `${currentCategory} > ${item.name}` : item.name;
      traverse(item.item, catName);
    } else if (item.request) {
      const path = (item.request.url.path || []).join('/');
      allEndpoints.push({
        category: currentCategory || '默认分类',
        name: item.name,
        method: item.request.method,
        path: '/' + path,
        bodyExample: item.request.body ? item.request.body.raw : '',
        description: item.request.description || ''
      });
    }
  }
}

traverse(data.item, '');

console.log(`Total endpoints found: ${allEndpoints.length}`);

// Group by category
const catMap = {};
for (const ep of allEndpoints) {
  if (!catMap[ep.category]) catMap[ep.category] = [];
  catMap[ep.category].push(ep);
}

for (const [cat, eps] of Object.entries(catMap)) {
  console.log(`\n### ${cat} (${eps.length})`);
  for (const ep of eps) {
    console.log(`  ${ep.method} ${ep.path} - ${ep.name}`);
  }
}

// Write summary to JSON for easy reference
fs.writeFileSync('d:\\WeChat-Hook\\tools\\showdoc_endpoints_summary.json', JSON.stringify(allEndpoints, null, 2), 'utf8');
console.log('\nWrote summary to tools\\showdoc_endpoints_summary.json');
