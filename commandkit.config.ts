import { tasks } from "@commandkit/tasks";
import { defineConfig } from "commandkit";

export default defineConfig({
	plugins: [
		tasks({
			sqliteDriverDatabasePath: "./tasks.db",
		}),
	],
});
