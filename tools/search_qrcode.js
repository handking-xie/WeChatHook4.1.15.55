async function searchAll() {
  const r = await fetch('http://127.0.0.1:30001/QueryDB/GetAllDBName', { method: 'POST' });
  const dbs = await r.json();
  for (const db of dbs) {
    try {
      const q = await fetch('http://127.0.0.1:30001/QueryDB/execute', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          db_name: db.dbName,
          sql: "SELECT name, sql FROM sqlite_master WHERE sql LIKE '%ticket%' OR sql LIKE '%qr%'"
        })
      });
      const res = await q.json();
      if (res.data && res.data.length) {
        console.log(`[${db.dbName}]`, res.data);
      }
    } catch (e) {}
  }
}

searchAll().catch(console.error);
