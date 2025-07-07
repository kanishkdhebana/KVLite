import time
import random
import string
import mysql.connector
from pymongo import MongoClient
import socket
import struct
import subprocess
from dotenv import load_dotenv
import os

# print("\nRunning custom key-value store benchmark...")
# subprocess.run(["./a.out"])


load_dotenv()

# Get database credentials from environment variables
db_user = os.getenv('DB_USER')
db_password = os.getenv('DB_PASSWORD')
db_host = os.getenv('DB_HOST')
db_database = os.getenv('DB_DATABASE')

NUM_ITERATIONS = 10000
TEST_KEYS = [f"key_{i}" for i in range(NUM_ITERATIONS)]
TEST_VALUES = [''.join(random.choices(string.ascii_letters + string.digits, k=16)) for _ in range(NUM_ITERATIONS)]

SERVER_ADDR = ('127.0.0.1', 1234)
MAX_MESSAGE = 4096

def send_all(sock, data):
    total_sent = 0
    while total_sent < len(data):
        sent = sock.send(data[total_sent:])
        if sent <= 0:
            raise RuntimeError("socket connection broken while sending")
        total_sent += sent

def recv_all(sock, size):
    data = b''
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if chunk == b'':
            raise RuntimeError("socket connection broken while reading")
        data += chunk
    return data

def send_request(sock, cmd):
    num_args = len(cmd)
    payload = struct.pack('!I', num_args)
    for arg in cmd:
        arg_bytes = arg.encode()
        payload += struct.pack('!I', len(arg_bytes)) + arg_bytes

    if len(payload) > MAX_MESSAGE:
        raise ValueError("Message too long")

    msg = struct.pack('!I', len(payload)) + payload
    send_all(sock, msg)

def read_response(sock):
    len_buf = recv_all(sock, 4)
    resp_len, = struct.unpack('!I', len_buf)
    if resp_len == 0 or resp_len > 1024 * 1024:
        raise ValueError(f"Invalid response length: {resp_len}")
    _ = recv_all(sock, resp_len)  # Ignore response content

def bench_custom_kv():
    print("\n--- Benchmarking custom key-value store (SET/GET/DEL) ---")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(SERVER_ADDR)

    times = {'set': 0, 'get': 0, 'delete': 0}
    # SET
    for k, v in zip(TEST_KEYS, TEST_VALUES):
        start = time.perf_counter()
        send_request(sock, ["set", k, v])
        read_response(sock)
        times['set'] += time.perf_counter() - start
    # GET
    for k in TEST_KEYS:
        start = time.perf_counter()
        send_request(sock, ["get", k])
        read_response(sock)
        times['get'] += time.perf_counter() - start
    # DEL
    for k in TEST_KEYS:
        start = time.perf_counter()
        send_request(sock, ["del", k])
        read_response(sock)
        times['delete'] += time.perf_counter() - start

    print(f"Avg set time: {times['set']/NUM_ITERATIONS*1000:.3f} ms")
    print(f"Avg get time: {times['get']/NUM_ITERATIONS*1000:.3f} ms")
    print(f"Avg delete time: {times['delete']/NUM_ITERATIONS*1000:.3f} ms")
    sock.close()



def bench_mariadb():
    print("\n--- Benchmarking MariaDB ---")
    conn = mysql.connector.connect(user=db_user, password=db_password, database=db_database, host=db_host)
    cur = conn.cursor()
    cur.execute("CREATE TABLE IF NOT EXISTS kv (k VARCHAR(255) PRIMARY KEY, v TEXT)")

    times = {'set': 0, 'get': 0, 'delete': 0}
    for k, v in zip(TEST_KEYS, TEST_VALUES):
        start = time.perf_counter()
        cur.execute("REPLACE INTO kv (k, v) VALUES (%s, %s)", (k, v))
        conn.commit()
        times['set'] += time.perf_counter() - start

        start = time.perf_counter()
        cur.execute("SELECT v FROM kv WHERE k = %s", (k,))
        cur.fetchone()
        times['get'] += time.perf_counter() - start

        start = time.perf_counter()
        cur.execute("DELETE FROM kv WHERE k = %s", (k,))
        conn.commit()
        times['delete'] += time.perf_counter() - start

    cur.close()
    conn.close()
    print(f"Avg set time: {times['set']/NUM_ITERATIONS*1000:.3f} ms")
    print(f"Avg get time: {times['get']/NUM_ITERATIONS*1000:.3f} ms")
    print(f"Avg delete time: {times['delete']/NUM_ITERATIONS*1000:.3f} ms")

def bench_mongodb():
    print("\n--- Benchmarking MongoDB ---")
    client = MongoClient('localhost', 27017)
    db = client.test
    collection = db.kv

    times = {'set': 0, 'get': 0, 'delete': 0}
    for k, v in zip(TEST_KEYS, TEST_VALUES):
        start = time.perf_counter()
        collection.replace_one({'_id': k}, {'_id': k, 'v': v}, upsert=True)
        times['set'] += time.perf_counter() - start

        start = time.perf_counter()
        collection.find_one({'_id': k})
        times['get'] += time.perf_counter() - start

        start = time.perf_counter()
        collection.delete_one({'_id': k})
        times['delete'] += time.perf_counter() - start

    print(f"Avg set time: {times['set']/NUM_ITERATIONS*1000:.3f} ms")
    print(f"Avg get time: {times['get']/NUM_ITERATIONS*1000:.3f} ms")
    print(f"Avg delete time: {times['delete']/NUM_ITERATIONS*1000:.3f} ms")

if __name__ == "__main__":
    bench_mariadb()
    bench_mongodb()
    bench_custom_kv()
