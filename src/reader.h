//
// Created by Kate Rupar on 4/5/20.
//

#pragma once

#include "row.h"
#include "SImap.h"

class Reader {
public:
    Reader() {

    }

    /** Reads from the given Row **/
    virtual bool visit(Row &) {}
};

class Adder : public Reader {
public:
    SIMap &map_;  // String to Num map;  Num holds an int

    Adder(SIMap &map) : map_(map) {}

    /** Reads from the given Row and adds elements to map **/
    bool visit(Row &r) override {
        r.printRow();
        String *word = r.get_string(0);
        assert(word != nullptr);
        Num *num = map_.contains(*word) ? map_.get(*word) : new Num();
        assert(num != nullptr);
        num->v++;
        map_.set(*word, num);
        return false;
    }
};