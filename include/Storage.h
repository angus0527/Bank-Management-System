//
// Created by huang on 2026/10/21.
//

#ifndef STORAGE_H
#define STORAGE_H

class Bank;

class Storage {
public:
    virtual ~Storage() = default;
    virtual void load(Bank& bank) = 0;
    virtual void save(const Bank& bank) = 0;
};

#endif //STORAGE_H
