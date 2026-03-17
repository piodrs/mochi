import { defineConfig } from "commandkit";
import { tasks } from "@commandkit/tasks";

export default defineConfig({
    plugins: [
        tasks({
            sqliteDriverDatabasePath: "./tasks.db",
        }),
    ],
});
