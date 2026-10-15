async function searchAllDbValues() {
  const r = await fetch('http://127.0.0.1:30001/QueryDB/GetAllDBName', { method: 'POST' });
  const dbs = await r.json();
  console.log('Searching all DBs for u.wechat.com...');

  for (const db of dbs) {
    try {
      const q = await fetch('http://127.0.0.1:30001/QueryDB/execute', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          db_name: db.dbName,
          sql: "SELECT name, sql FROM sqlite_master WHERE type='table'"
        })
      });
      const res = await q.json();
      if (!res.data) continue;

      for (const t of res.data) {
        if (!t.name || t.name.startsWith('sqlite_') || t.name.startsWith('Msg_') || t.name.includes('fts')) continue;
        try {
          // Check if table has text columns
          const pragmaRes = await fetch('http://127.0.0.1:30001/QueryDB/execute', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              db_name: db.dbName,
              sql: `PRAGMA table_info(${t.name})`
            })
          });
          const colsData = await pragmaRes.json();
          if (!colsData.data) continue;
          const textCols = colsData.data.filter(c => c.type === 'TEXT').map(c => c.name);
          if (textCols.length === 0) continue;

          const whereClauses = textCols.map(c => `(${c} LIKE '%u.wechat.com%' OR ${c} LIKE '%wechat.com/M%')`).join(' OR ');
          const searchSql = `SELECT * FROM ${t.name} WHERE ${whereClauses} LIMIT 5`;

          const hitRes = await fetch('http://127.0.0.1:30001/QueryDB/execute', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              db_name: db.dbName,
              sql: searchSql
            })
          });
          const hitData = await hitRes.json();
          if (hitData.data && hitData.data.length > 0) {
            console.log(`[FOUND in ${db.dbName} -> ${t.name}]:`, hitData.data);
          }
        } catch(e) {}
      }
    } catch (e) {}
  }
  console.log('Search completed.');
}

searchAllDbValues().catch(console.error);
