#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <libpq-fe.h>

// note: error handling omitted for simplicity
const char ** embed(const char **input, size_t input_size, char *api_key) {
    // prepare request

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/embeddings");

    // prepare request data

    cJSON *request_json = cJSON_CreateObject();
    cJSON *input_json = cJSON_AddArrayToObject(request_json, "input");
    for (size_t i = 0; i < input_size; i++) {
        cJSON_AddItemToArray(input_json, cJSON_CreateStringReference(input[i]));
    }
    cJSON_AddStringToObject(request_json, "model", "text-embedding-3-small");
    char *request_data = cJSON_PrintUnformatted(request_json);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_data);

    // prepare request headers

    struct curl_slist *headers = NULL;
    char authorization[1024];
    snprintf(authorization, sizeof(authorization), "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, authorization);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // prepare response stream

    char *response_data;
    size_t unused;
    FILE *response_stream = open_memstream(&response_data, &unused);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response_stream);

    // perform request

    CURLcode res = curl_easy_perform(curl);
    assert(res == CURLE_OK);

    // check response code

    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    assert(response_code == 200);

    // process response data

    fclose(response_stream);
    cJSON *response_json = cJSON_Parse(response_data);
    cJSON *objects = cJSON_GetObjectItemCaseSensitive(response_json, "data");

    const char **embeddings = malloc(sizeof(char *) * input_size);
    for (size_t i = 0; i < input_size; i++) {
        cJSON *object = cJSON_GetArrayItem(objects, i);
        cJSON *embedding = cJSON_GetObjectItemCaseSensitive(object, "embedding");
        embeddings[i] = cJSON_PrintUnformatted(embedding);
    }

    // free data

    cJSON_Delete(request_json);
    free(request_data);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(response_data);
    cJSON_Delete(response_json);

    return embeddings;
}

int main(void) {
    char *api_key = getenv("OPENAI_API_KEY");
    if (!api_key) {
        printf("Set OPENAI_API_KEY\n");
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    PGconn *conn = PQconnectdb("postgres://localhost/pgvector_example");
    assert(PQstatus(conn) == CONNECTION_OK);

    PGresult *res = PQexec(conn, "CREATE EXTENSION IF NOT EXISTS vector");
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);

    res = PQexec(conn, "DROP TABLE IF EXISTS documents");
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);

    res = PQexec(conn, "CREATE TABLE documents (id bigserial PRIMARY KEY, content text, embedding vector(1536))");
    assert(PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);

    const char *input[] = {
        "The dog is barking",
        "The cat is purring",
        "The bear is growling"
    };
    const char **embeddings = embed(input, 3, api_key);
    for (size_t i = 0; i < 3; i++) {
        const char *params[] = {input[i], embeddings[i]};
        res = PQexecParams(conn, "INSERT INTO documents (content, embedding) VALUES ($1, $2)", 2, NULL, params, NULL, NULL, 0);
        assert(PQresultStatus(res) == PGRES_COMMAND_OK);
        PQclear(res);
        free((void *)embeddings[i]);
    }
    free((void *)embeddings);

    const char *query[] = {"forest"};
    const char **query_embeddings = embed(query, 1, api_key);
    const char *query_params[] = {query_embeddings[0]};
    res = PQexecParams(conn, "SELECT content FROM documents ORDER BY embedding <=> $1 LIMIT 5", 1, NULL, query_params, NULL, NULL, 0);
    assert(PQresultStatus(res) == PGRES_TUPLES_OK);
    int ntuples = PQntuples(res);
    for (int i = 0; i < ntuples; i++) {
        printf("%s\n", PQgetvalue(res, i, 0));
    }
    PQclear(res);
    free((void *)query_embeddings[0]);
    free((void *)query_embeddings);

    curl_global_cleanup();

    return 0;
}
