#pragma once

#include <cstddef>

struct Datum
{
    size_t id;
    double x;
    double y;
    double z;
};

class DataContainerImpl;

class DataContainer
{
    DataContainerImpl *m_pimpl;

public:
    Datum &at(size_t index);
    void push_back(Datum datum);
    size_t size() const;

    DataContainer(size_t id);
    DataContainer(const DataContainer &) = delete;
    ~DataContainer();
};
