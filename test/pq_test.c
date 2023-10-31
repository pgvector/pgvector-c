#include <assert.h>
#include <libpq-fe.h>

int main() {
    PGconn *conn;
    PGresult *res;

    conn = PQconnectdb("postgres://localhost/pgvector_c_test");
    assert(PQstatus(conn) == CONNECTION_OK);

    res = PQexec(conn, "CREATE EXTENSION IF NOT EXISTS vector");
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);

    res = PQexec(conn, "DROP TABLE IF EXISTS items");
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);

    res = PQexec(conn, "CREATE TABLE items (id bigserial PRIMARY KEY, embedding vector(3))");
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);

    const char *paramValues[3] = {"[1,1,1]", "[2,2,2]", "[1,1,2]"};
    res = PQexecParams(conn, "INSERT INTO items (embedding) VALUES ($1), ($2), ($3)", 3, NULL, paramValues, NULL, NULL, 0);
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);

    const char *paramValues2[1] = {"[1,1,1]"};
    res = PQexecParams(conn, "SELECT * FROM items ORDER BY embedding <-> $1 LIMIT 5", 1, NULL, paramValues2, NULL, NULL, 0);
    assert(PQresultStatus(res) == PGRES_TUPLES_OK);
    int ntuples = PQntuples(res);
    for (int i = 0; i < ntuples; i++) {
        printf("%s: %s\n", PQgetvalue(res, i, 0), PQgetvalue(res, i, 1));
    }
    PQclear(res);

    res = PQexec(conn, "CREATE INDEX ON items USING hnsw (embedding vector_l2_ops)");
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);

    PQfinish(conn);

    return 0;
}
