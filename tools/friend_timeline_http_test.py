"""Bounded read-only production-route QA. Logs counts and checks, never timeline content.

Selects one user from the existing local timeline service; no account IDs are saved.
Start the native diagnostic server separately with nickname_native_runner.py --http.
"""
import argparse
import json
import time
import urllib.request


def post(port, path, body):
    request = urllib.request.Request(
        f'http://127.0.0.1:{port}/{path}',
        data=body.encode('utf-8'), headers={'Content-Type': 'application/json'})
    with urllib.request.urlopen(request, timeout=32) as response:
        return json.load(response)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', type=int, default=30089)
    parser.add_argument('--cache-port', type=int, default=30001)
    args = parser.parse_args()
    for attempt in range(20):
        try:
            invalid = post(args.port, 'GetFriendTimeline', '{}')
            break
        except OSError:
            if attempt == 19:
                raise
            time.sleep(0.2)
    assert invalid['ret'] == -1 and not invalid['submitted']
    for body in ('{', '[]', '{"wxid":1}', '{"wxid":"x","id":1}',
                 '{"wxid":"x","id":"18446744073709551616"}',
                 '{"wxid":"x","count":1.5}', '{"wxid":"x\\u0000y"}'):
        invalid = post(args.port, 'GetFriendTimeline', body)
        assert invalid['ret'] == -1 and not invalid['submitted'] and not invalid['verified']
        assert invalid['data'] == []
    cache = post(args.cache_port, 'TimelineGetFristPage', '{"count":1}')
    assert cache['ret'] == 0 and cache['data'], 'no cached friend available for read-only test'
    target = cache['data'][0]['user_name']

    def page(path, cursor='', count=3):
        response = post(args.port, path, json.dumps({'wxid': target, 'id': cursor, 'count': count}))
        assert response['ret'] == 0, response.get('retmsg')
        assert response['source'] == 'network' and response['submitted'] and response['verified']
        assert response['count'] == len(response['data']) <= count
        assert all(item['user_name'] == target for item in response['data'])
        times = [int(item['create_time']) for item in response['data']]
        assert times == sorted(times, reverse=True)
        if cursor:
            assert all(int(item['sns_id']) < int(cursor) for item in response['data'])
        return response

    first = page('GetFriendTimeline')
    second = page('GetFriendTimeline', first['next_id']) if first['data'] else None
    alias = page('mmsnsuserpage', count=1)
    first_ids = {item['sns_id'] for item in first['data']}
    assert not second or first_ids.isdisjoint(item['sns_id'] for item in second['data'])
    assert not alias['data'] or alias['data'][0]['sns_id'] in first_ids
    print(json.dumps({'passed': True, 'first_count': first['count'],
        'next_count': second['count'] if second else None, 'alias_count': alias['count'],
        'source': 'network', 'no_duplicates': True, 'invalid_parameters_rejected': True}))


if __name__ == '__main__':
    main()
