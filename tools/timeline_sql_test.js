// Execute the SQL expressions extracted from production C++ against isolated SQLite fixtures.
const fs = require('node:fs');
const assert = require('node:assert/strict');
const { DatabaseSync } = require('node:sqlite');
const source = fs.readFileSync(require('node:path').join(__dirname, '../src/Timeline.cpp'), 'utf8');
const expressions = [...source.matchAll(/sql = ([\s\S]*?);/g)].map(m => m[1]);
assert.equal(expressions.length, 4);
function query(index, targetUser, nextId = '', xmlCursor = nextId) {
    const values = { targetUser, nextId, xmlCursor };
    return expressions[index].match(/"(?:\\.|[^"\\])*"|std::to_string\(count\)|targetUser|nextId|xmlCursor/g)
        .map(t => t.startsWith('"') ? JSON.parse(t) : t === 'std::to_string(count)' ? '2' : values[t]).join('');
}
const db = new DatabaseSync(':memory:');
db.exec(`
CREATE TABLE SnsTimeLine(tid TEXT, user_name TEXT, content TEXT);
CREATE TABLE SnsMainTimeLineBreakFlag(tid TEXT, tid_heigh_bit INTEGER, tid_low_bit INTEGER);
CREATE TABLE SnsUserTimeLineBreakFlagV2(tid TEXT, user_name TEXT, tid_heigh_bit INTEGER, tid_low_bit INTEGER);
`);
for (const [tid, user, id] of [['5', 'friend', '105'], ['4', 'other', '104'], ['3', 'friend', '103'], ['2', 'friend', '102'], ['1', "x' OR 1=1 --", "literal%_\\'"]]) {
    db.prepare('INSERT INTO SnsTimeLine VALUES (?,?,?)').run(tid, user, `<id>${id}</id>`);
    db.prepare('INSERT INTO SnsMainTimeLineBreakFlag VALUES (?,0,?)').run(tid, Number(tid));
    db.prepare('INSERT INTO SnsUserTimeLineBreakFlagV2 VALUES (?,?,0,?)').run(tid, user, Number(tid));
}
const ids = sql => db.prepare(sql).all().map(row => row.tid);
assert.deepEqual(ids(query(3, 'friend')), ['5', '3']);
assert.deepEqual(ids(query(2, 'friend', '103')), ['2']);
assert.deepEqual(ids(query(2, 'friend', '3')), ['2']);
assert.deepEqual(ids(query(2, 'friend', '104')), []); // other user's cursor
assert.deepEqual(ids(query(2, 'friend', 'missing')), []);
assert.deepEqual(ids(query(3, "x'' OR 1=1 --")), ['1']);
assert.deepEqual(ids(query(0, '', "literal%_\\''", "literal\\%\\_\\\\''")), []);
assert.deepEqual(ids(query(1, '')), ['5', '4']);
db.close();
console.log('Timeline production SQL tests passed');
