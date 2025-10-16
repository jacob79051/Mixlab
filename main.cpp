//#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "external/httplib.h"
#include "external/sqlite3.h"
#include <iostream>

int main() {
    // Initialize SQLite
    sqlite3* db;
    if (sqlite3_open("data.db", &db)) {
        std::cerr << "Failed to open database\n";
        return 1;
    }

    // Create a table if not exists
    // Should be moved to a file if possible
    const char* create_sql = R"(
    CREATE TABLE IF NOT EXISTS Artists (
        artist_id TEXT PRIMARY KEY,
        name TEXT NOT NULL,
        artist_tags TEXT
    );

    CREATE TABLE IF NOT EXISTS Songs (
        song_id TEXT PRIMARY KEY,
        title TEXT NOT NULL,
        artist_id TEXT NOT NULL,
        year INT NOT NULL,
        FOREIGN KEY (artist_id) REFERENCES Artists(artist_id)
    );

    CREATE TABLE IF NOT EXISTS SongData (
        song_id TEXT PRIMARY KEY,
        tempo REAL CHECK (tempo > 0),
        duration REAL CHECK (duration > 0),
        key INT CHECK (key >= 0 AND key <= 11),
        mode BOOLEAN,
        time_signature INT CHECK (time_signature > 0),
        loudness REAL,
        FOREIGN KEY (song_id) REFERENCES Songs(song_id)
    );
    )";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, create_sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error creating tables: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }

    // Create server
    httplib::Server svr;

    // The HTML for the server, should be moved to its own file if possible
    svr.Get("/", [&](const httplib::Request&, httplib::Response& res) {
        std::string html = R"(
            <html>
            <head>
                <title>Mixlab</title>
                <style>
                    body { font-family: Arial; margin: 2em; background-color: #f7f7f7; }
                    h1 { color: #333; }
                    textarea { width: 100%; height: 100px; font-family: monospace; }
                    button { margin-top: 10px; padding: 10px 15px; font-size: 16px; }
                    pre { background: #eee; padding: 10px; border-radius: 5px; }
                </style>
            </head>
            <body>
                <h1>Mixlab</h1>
                <form method="POST" action="/query">
                    <textarea name="sql"></textarea><br>
                    <button type="submit">Run Query</button>
                </form>
            </body>
            </html>
        )";
        res.set_content(html, "text/html");
    });


    // Endpoint for query result
    svr.Post("/query", [&](const httplib::Request& req, httplib::Response& res) {
        std::string sql = req.get_param_value("sql");
        std::string output;

        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

        // Output errors
        if (rc != SQLITE_OK) {
            res.set_content("SQL error: " + std::string(sqlite3_errmsg(db)), "text/plain");
            return;
        }

        // Detect type of query
        if (sqlite3_column_count(stmt) > 0) {
            // SELECT query
            output += "<pre>";
            int colCount = sqlite3_column_count(stmt);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                for (int i = 0; i < colCount; i++) {
                    const char* val = (const char*)sqlite3_column_text(stmt, i);
                    output += val ? val : "NULL";
                    if (i < colCount - 1) output += " | ";
                }
                output += "\n";
            }
            output += "</pre>";
        } else {
            // Any other query
            sqlite3_step(stmt);
            output = "Query executed successfully.";
        }

        sqlite3_finalize(stmt);

        // Format the output with html
        std::string html = "<html><body><h2>Query Result</h2>" + output +
                        "<br><a href='/'>Back</a></body></html>";
        res.set_content(html, "text/html");
    });

    std::cout << "Server running on http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);

    sqlite3_close(db);
}
