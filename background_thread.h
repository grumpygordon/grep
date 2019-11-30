#ifndef BACKGROUND_THREAD_H
#define BACKGROUND_THREAD_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

class background_thread : public QObject {
    Q_OBJECT

    mutable std::mutex m;
    QString pattern, file;
    QString f_pattern, f_file;
    bool quit{}, callback_queued{}, has_work{};
    std::atomic_bool cancel{};
    std::condition_variable has_work_cv;
    std::thread thread;

public:
    struct result_t {
        std::vector<QString> matches;
        size_t found_matches{}, completed_files{};
        bool finished{}, trouble{};
        QString bad_file;
    };

    background_thread();

    ~background_thread();

    void set_pattern(QString const& new_pattern);

    void set_file(QString const& new_filename);

    void start();

    void stop();

    result_t get_result() const;

signals:

    void result_changed();

private:
    void enqueue_callback();

    Q_INVOKABLE void callback();

    void work(QString const& pat, QString const& path, std::vector<int> &pfun);

    result_t current_result;
};
#endif // BACKGROUND_THREAD_H
