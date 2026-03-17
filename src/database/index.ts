import fs from "node:fs";
import { Logger } from "commandkit";
import { cert, getApp, getApps, initializeApp } from "firebase-admin/app";
import { getFirestore } from "firebase-admin/firestore";
import { env } from "@/env";

function readServiceAccount() {
	const fileContent = fs.readFileSync(env.FIREBASE_PATH, { encoding: "utf-8" });
	return JSON.parse(fileContent);
}

function ensureFirebaseApp() {
	const existing = getApps()[0];

	if (existing) return getApp();

	return initializeApp({
		credential: cert(readServiceAccount()),
	});
}

try {
	const app = ensureFirebaseApp();
	getFirestore(app);
	Logger.info("Firestore initialized successfully");
} catch (error) {
	Logger.error(error);
	process.exit(1);
}
