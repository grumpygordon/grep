#include "background_thread.h"
#include <iostream>
#include <QtCore/QDirIterator>
#include <QDir>

background_thread::background_thread() : thread([this] {
    while (true) {
        std::unique_lock<std::mutex> lg(m);
        has_work_cv.wait(lg, [this] {
            return quit || has_work;
        });
        if (quit)
            return;
        QString cur_pattern = pattern, cur_file = file;

        current_result.finished = true;
        current_result.matches.clear();
        current_result.found_matches = 0;
        current_result.completed_files = 0;
        current_result.trouble = false;
        current_result.file = cur_file;
        enqueue_callback();
        if (QDir(cur_file).exists() && QFile(cur_file).exists()) {
            lg.unlock();
            std::vector<int> pfun;
            int p_size = cur_pattern.size();

            pfun.reserve(size_t(p_size + 1));
            int k = -1;
            pfun.push_back(k);
            for (auto ch : cur_pattern) {
                if (cancel.load())
                    break;
                while (k >= 0 && ch != cur_pattern[k])
                    k = pfun[size_t(k)];
                k++;
                pfun.push_back(k);
            }
            try {
                work(cur_pattern, cur_file, pfun);
            } catch (std::exception const& e) {
                std::cerr << e.what() << '\n';
                lg.lock();
                if (!current_result.trouble) {
                    current_result.trouble = true;
                    current_result.bad_file = cur_file;
                }
                lg.unlock();
            }
            lg.lock();
        }
        current_result.finished = cancel.load();
        enqueue_callback();

        has_work = false;
        cancel.store(false);
    }
}) {}

background_thread::~background_thread() {
    cancel.store(true);
    {
        std::lock_guard<std::mutex> lg(m);
        quit = true;
        has_work_cv.notify_all();
    }
    thread.join();
}

void background_thread::set_pattern(QString const& new_pattern) {
    std::lock_guard<std::mutex> lg(m);
    f_pattern = new_pattern;
}

void background_thread::set_file(QString const& new_file) {
    std::lock_guard<std::mutex> lg(m);
    f_file = new_file;
}

void background_thread::start() {
    std::lock_guard<std::mutex> lg(m);
    if (has_work) {
        return;
    }
    has_work = true;
    pattern = f_pattern;
    file = f_file;
    has_work_cv.notify_all();
}

void background_thread::stop() {
    std::unique_lock<std::mutex> lg(m);
     cancel.store(true);
}

background_thread::result_t background_thread::get_result() const {
    std::lock_guard<std::mutex> lg(m);
    return current_result;
}

void background_thread::enqueue_callback() {
    if (callback_queued) {
        return;
    }
    callback_queued = true;
    QMetaObject::invokeMethod(this, "callback", Qt::QueuedConnection);
}

void background_thread::callback() {
    {
        std::lock_guard<std::mutex> lg(m);
        callback_queued = false;
    }
  emit result_changed();
}

void background_thread::work(QString const& pat, QString const& path, std::vector<int> &pfun) {
    if (cancel.load())
        return;
    QDir directory(path);
    if (directory.exists()) {
        QDirIterator it(path,
                    QDir::Files | QDir::NoSymLinks | QDir::Dirs |
                    QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Readable,
                    QDirIterator::Subdirectories);
        while (it.hasNext()) {
            if (cancel.load())
                return;
            QString file = it.next();
            try {
                work(pat, file, pfun);
            } catch (...) {
                std::unique_lock<std::mutex> lg(m);
                if (!current_result.trouble) {
                    current_result.trouble = true;
                    current_result.bad_file = file;
                }
                lg.unlock();
                throw;
            }
        }
        return;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return;
    size_t constexpr BUFFER_SIZE = 1u << 20u;
    size_t constexpr MAX_RESULTS = 100;

    QTextStream stream(&f);
    QString buffer;

    int k = 0, line = 1, pos = 0;
    if (cancel.load())
        return;
    while (!stream.atEnd()) {
        buffer = stream.read(BUFFER_SIZE);
        if (cancel.load())
            return;
        for (auto ch : buffer) {
            if (ch == '\n') {
                ++line;
                pos = 0;
            }
            while (k >= 0 && (k == pat.size() || ch != pat[k]))
                k = pfun[size_t(k)];
            k++;
            if (k == pat.size()) {
                std::lock_guard<std::mutex> lg(m);
                ++current_result.found_matches;
                if (current_result.matches.size() < MAX_RESULTS) {
                    QString res = QString("%1 %2:%3").arg(path,
                                                    QString::number(line),
                                                    QString::number(pos + 1 - pat.size()));
                    current_result.matches.push_back(res);
                }
                enqueue_callback();
            }
            ++pos;
        }
    }
    std::lock_guard<std::mutex> lg(m);
    ++current_result.completed_files;
    enqueue_callback();
}
