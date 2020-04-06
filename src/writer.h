//
// Created by Kate Rupar on 4/5/20.
//

#ifndef MILESTONE1_WRITER_H
#define MILESTONE1_WRITER_H

#endif //MILESTONE1_WRITER_H

#include "row.h"
#include "args.h"

class Writer {
public:
    Writer() {

    }
    virtual void visit(Row&) {}
    virtual bool done() {}
};


class FileReader : public Writer {
public:
    /** Reads next word and stores it in the row. Actually read the word.
        While reading the word, we may have to re-fill the buffer  */
    void visit(Row & r) override {
        cout << "I:" << this->i_ << endl;
        cout << "end:" << this->end_ << endl;
        assert(i_ < end_);
        assert(! isspace(buf_[i_]));
        size_t wStart = i_;
        while (true) {
            if (i_ == end_) {
                if (feof(file_)) { ++i_;  break; }
                i_ = wStart;
                wStart = 0;
                fillBuffer_();
            }
            if (isspace(buf_[i_]))  break;
            ++i_;
        }
        buf_[i_] = 0;
        cout << "finished loop" << endl;
        String* word = new String(buf_ + wStart, i_ - wStart);
        cout <<word->cstr_ << endl;
        r.set(0, word);
        ++i_;
        skipWhitespace_();
    }

    /** Returns true when there are no more words to read.  There is nothing
       more to read if we are at the end of the buffer and the file has
       all been read.     */
    bool done() override {
        cout << "Checking Done: i=" << i_ << " end=" << end_ << " eof?=" << feof(file_);
        return (i_ >= end_) && feof(file_);
    }

    /** Creates the reader and opens the file for reading.  */
    FileReader() {
        file_ = fopen(arg.file, "r");
        if (file_ == nullptr) cout << "Cannot open file " << arg.file << endl;
        buf_ = new char[BUFSIZE + 1]; //  null terminator
        fillBuffer_();
        skipWhitespace_();
        cout << "FR CREATED" << endl;
    }

    static const size_t BUFSIZE = 1024;

    /** Reads more data from the file. */
    void fillBuffer_() {
        size_t start = 0;
        // compact unprocessed stream
        if (i_ != end_) {
            start = end_ - i_;
            memcpy(buf_, buf_ + i_, start);
        }
        // read more contents
        end_ = start + fread(buf_+start, sizeof(char), BUFSIZE - start, file_);
        i_ = start;
    }

    /** Skips spaces.  Note that this may need to fill the buffer if the
        last character of the buffer is space itself.  */
    void skipWhitespace_() {
        while (true) {
            if (i_ == end_) {
                if (feof(file_)) return;
                fillBuffer_();
            }
            // if the current character is not whitespace, we are done
            if (!isspace(buf_[i_]))
                return;
            // otherwise skip it
            ++i_;
        }
    }

    char * buf_;
    size_t end_ = 0;
    size_t i_ = 0;
    FILE * file_;
};


