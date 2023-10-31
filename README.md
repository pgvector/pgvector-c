# pgvector-c

[pgvector](https://github.com/pgvector/pgvector) examples for C

Supports [libpq](https://www.postgresql.org/docs/current/libpq.html)

[![Build Status](https://github.com/pgvector/pgvector-c/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/pgvector/pgvector-c/actions)

## Getting Started

Follow the instructions for your database library:

- [libpq](#libpq)

## libpq

Enable the extension

```c
PGresult *res = PQexec(conn, "CREATE EXTENSION IF NOT EXISTS vector");
```

Create a table

```c
PGresult *res = PQexec(conn, "CREATE TABLE items (id bigserial PRIMARY KEY, embedding vector(3))");
```

Insert vectors

```c
const char *paramValues[2] = {"[1,2,3]", "[4,5,6]"};
PGresult *res = PQexecParams(conn, "INSERT INTO items (embedding) VALUES ($1), ($2)", 2, NULL, paramValues, NULL, NULL, 0);
```

Get the nearest neighbors

```c
const char *paramValues[1] = {"[3,1,2]"};
PGresult *res = PQexecParams(conn, "SELECT * FROM items ORDER BY embedding <-> $1 LIMIT 5", 1, NULL, paramValues, NULL, NULL, 0);
```

Add an approximate index

```c
PGresult *res = PQexec(conn, "CREATE INDEX ON items USING ivfflat (embedding vector_l2_ops) WITH (lists = 100)");
// or
PGresult *res = PQexec(conn, "CREATE INDEX ON items USING hnsw (embedding vector_l2_ops)");
```

Use `vector_ip_ops` for inner product and `vector_cosine_ops` for cosine distance

See a [full example](test/pq_test.c)

## Contributing

Everyone is encouraged to help improve this project. Here are a few ways you can help:

- [Report bugs](https://github.com/pgvector/pgvector-c/issues)
- Fix bugs and [submit pull requests](https://github.com/pgvector/pgvector-c/pulls)
- Write, clarify, or fix documentation
- Suggest or add new features

To get started with development:

```sh
git clone https://github.com/pgvector/pgvector-c.git
cd pgvector-c
createdb pgvector_c_test
gcc -Wall -Wextra -Werror -o test/pq test/pq_test.c -lpq
test/pq
```
