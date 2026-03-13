const axios = require("axios");
const cheerio = require("cheerio");
const fs = require("fs");


// ===== CONFIG =====

const START_URL = "https://vnexpress.net";

const OUTPUT_FILE = "corpus.txt";

const MAX_ARTICLES = 100000;

const CONCURRENCY = 5;

const MAX_QUEUE = 5000;


// ===== STATE =====

const visited = new Set();
const queue = [START_URL];
const sentenceSet = new Set();

let articleCount = 0;
let buffer = [];


// ===== CLEAN TEXT =====

function cleanText(text) {

    text = text.toLowerCase();

    text = text.replace(/[^a-zàáạảãâầấậẩẫăằắặẳẵèéẹẻẽêềếệểễìíịỉĩòóọỏõôồốộổỗơờớợởỡùúụủũưừứựửữỳýỵỷỹđ\s]/g, "");

    text = text.replace(/\s+/g, " ").trim();

    return text;
}


// ===== SPLIT SENTENCE =====

function splitSentences(text) {

    return text
        .split(/[.!?]/)
        .map(s => cleanText(s))
        .filter(s => s.length > 10);
}


// ===== SAVE BUFFER =====

function flushBuffer() {

    if (buffer.length === 0) return;

    fs.appendFileSync(OUTPUT_FILE, buffer.join("\n") + "\n");

    buffer = [];
}


// ===== URL FILTER =====

function isArticleUrl(url) {

    if (!url.includes("vnexpress.net")) return false;

    if (!url.includes(".html")) return false;

    return true;
}


// ===== EXTRACT ARTICLE =====

function extractArticle($) {

    let paragraphs = [];

    $("p.Normal").each((i, el) => {

        if ($(el).children().length > 0) return;

        const text = $(el).text().trim();

        if (text.length > 50) {
            paragraphs.push(text);
        }

    });

    return paragraphs.join(" ");
}


// ===== ADD SENTENCES =====

function addSentences(sentences) {

    for (let s of sentences) {

        if (sentenceSet.has(s)) continue;

        sentenceSet.add(s);

        buffer.push(s);

        if (buffer.length > 100) flushBuffer();
    }
}


// ===== CRAWL PAGE =====

async function crawlPage(url) {

    if (visited.has(url)) return;

    visited.add(url);

    try {

        const res = await axios.get(url, {
            timeout: 8000,
            headers: { "User-Agent": "Mozilla/5.0" }
        });

        const html = res.data;

        const $ = cheerio.load(html);

        // ===== ARTICLE =====

        if (isArticleUrl(url)) {

            const articleText = extractArticle($);

            if (articleText.length > 0) {

                const sentences = splitSentences(articleText);

                addSentences(sentences);

                articleCount++;

                console.log("Article:", articleCount, "| Sentences:", sentenceSet.size);
            }
        }

        // ===== FIND LINKS =====

        $("a[href*='vnexpress.net']").each((i, el) => {

            let link = $(el).attr("href");

            if (!link) return;

            if (link.startsWith("/")) {
                link = START_URL + link;
            }

            if (!isArticleUrl(link)) return;

            if (visited.has(link)) return;

            if (queue.length > MAX_QUEUE) return;

            queue.push(link);
        });

    } catch (err) {

        // ignore errors
    }
}


// ===== WORKER =====

async function worker() {

    while (articleCount < MAX_ARTICLES) {

        const url = queue.shift();

        if (!url) {

            await sleep(100);
            continue;
        }

        await crawlPage(url);

        await sleep(500); // delay mỗi request
    }
}


// ===== START =====

async function start() {

    console.log("Crawler started");

    const workers = [];

    for (let i = 0; i < CONCURRENCY; i++) {
        workers.push(worker());
    }

    await Promise.all(workers);

    flushBuffer();

    console.log("DONE");
}

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

start();