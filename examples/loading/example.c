#include <arpa/inet.h>
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpq-fe.h>

void write_int16(char *buffer, int *pos, int16_t v) {
    int16_t s = htons(v);
    memcpy(buffer + *pos, &s, sizeof(int16_t));
    *pos += sizeof(int16_t);
}

void write_int32(char *buffer, int *pos, int32_t v) {
    int32_t s = htonl(v);
    memcpy(buffer + *pos, &s, sizeof(int32_t));
    *pos += sizeof(int32_t);
}

void write_float(char *buffer, int *pos, float v) {
    write_int32(buffer, pos, *((int32_t *)&v));
}

int main(void) {
    // generate random data
    int rows = 1000000;
    int dimensions = 128;
    float **embeddings = malloc(rows * sizeof(float *));
    for (int i = 0; i < rows; i++) {
        float *embedding = malloc(dimensions * sizeof(float));
        for (int j = 0; j < dimensions; j++)
            embedding[j] = rand() / (float)INT_MAX;
        embeddings[i] = embedding;
    }

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
    res = PQexec(conn, "CREATE TABLE items (id bigserial, embedding vector(128))");
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);

    // load data
    printf("Loading %d rows\n", rows);
    res = PQexec(conn, "COPY items (embedding) FROM STDIN WITH (FORMAT BINARY)");
    assert(PQresultStatus(res) == PGRES_COPY_IN);
    PQclear(res);

    // write header
    // https://www.postgresql.org/docs/current/sql-copy.html
    char buffer[32768] = "PGCOPY\n\xFF\r\n\0";
    int pos = 11;
    write_int32(buffer, &pos, 0);
    write_int32(buffer, &pos, 0);
    assert(PQputCopyData(conn, buffer, pos) == 1);
    pos = 0;

    for (int i = 0; i < rows; i++) {
        // write row
        write_int16(buffer, &pos, 1);
        write_int32(buffer, &pos, 4 + dimensions * sizeof(float));
        write_int16(buffer, &pos, dimensions);
        write_int16(buffer, &pos, 0);
        for (int j = 0; j < dimensions; j++)
            write_float(buffer, &pos, embeddings[i][j]);

        assert(PQputCopyData(conn, buffer, pos) == 1);
        pos = 0;

        // show progress
        if (i % 10000 == 0) {
            printf(".");
            fflush(stdout);
        }
    }

    // write trailer
    write_int16(buffer, &pos, -1);
    assert(PQputCopyData(conn, buffer, pos) == 1);
    assert(PQputCopyEnd(conn, NULL) == 1);

    res = PQgetResult(conn);
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    printf("\nSuccess!\n");

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

    for (int i = 0; i < rows; i++)
        free(embeddings[i]);
    free(embeddings);

    return 0;
}
