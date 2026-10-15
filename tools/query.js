const db_name = process.argv[2] || 'contact.db';
const sql = process.argv[3] || 'SELECT name, sql FROM sqlite_master WHERE type="table"';

fetch('http://127.0.0.1:30001/QueryDB/execute', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ db_name, sql })
})
.then(r => r.json())
.then(data => {
  console.log(JSON.stringify(data, null, 2));
})
.catch(err => {
  console.error(err);
});
