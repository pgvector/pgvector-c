#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include <libpq-fe.h>

int main(void) {
    // TODO generate random data
    int rows = 100000;

    PGconn *conn = PQconnectdb("postgres://localhost/pgvector_example");
    assert(PQstatus(conn) == CONNECTION_OK);

    // enable extension
    PGresult *res = PQexec(conn, "CREATE EXTENSION IF NOT EXISTS vector");
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);

    res = PQexec(conn, "DROP TABLE IF EXISTS items");
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);

    // create table
    res = PQexec(conn, "CREATE TABLE items (id bigserial, embedding vector(3))");
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);

    // load data
    // TODO use binary format
    printf("Loading %d rows\n", rows);
    res = PQexec(conn, "COPY items (embedding) FROM STDIN");
    assert(PQresultStatus(res) == PGRES_COPY_IN);
    PQclear(res);

    for (int i = 0; i < rows; i++) {
        assert(PQputCopyData(conn, "[1,2,3]\n", 8) == 1);
    }
    assert(PQputCopyEnd(conn, NULL) == 1);

    res = PQgetResult(conn);
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    printf("Success!\n");

    // create any indexes *after* loading initial data (skipping for this example)
    bool create_index = false;
    if (create_index) {
        printf("Creating index\n");

        res = PQexec(conn, "SET maintenance_work_mem = '8GB'");
        assert(PQresultStatus(res) == PGRES_COMMAND_OK);
        PQclear(res);

        res = PQexec(conn, "SET max_parallel_maintenance_workers = 7");
        assert(PQresultStatus(res) == PGRES_COMMAND_OK);
        PQclear(res);

        res = PQexec(conn, "CREATE INDEX ON items USING hnsw (embedding vector_cosine_ops)");
        assert(PQresultStatus(res) == PGRES_COMMAND_OK);
        PQclear(res);
    }

    // update planner statistics for good measure
    res = PQexec(conn, "ANALYZE items");
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);

    PQfinish(conn);

    return 0;
}
