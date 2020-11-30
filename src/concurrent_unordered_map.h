/**
 * @file StreamStore.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-26
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <optional>

template <class K, class V>
class concurrent_unordered_map
{
public:
    std::optional<read_guard<V>> get(const K &key)
    {
        std::shared_lock lock(mutex);
        auto item = map.find(key);
        if (item == map.end()) {
            return std::nullopt;
        }
        return read_guard(item);
    }

    std::optional<V> get_copy(const K &key)
    {
        std::shared_lock lock(mutex);
        auto item = map.find(key);
        if (item == map.end()) {
            return std::nullopt;
        }
        read_guard(item);
        return *item;
    }

    std::optional<write_guard<V>> get_mutable(const K &key)
    {
        std::shared_lock lock(mutex);
        auto item = map.find(key);
        if (item == map.end()) {
            return std::nullopt;
        }
        return write_guard(item);
    }

    size_type erase(const K &key)
    {
        std::unique_lock lock(mutex);
        auto item = map.find(key);
        if (item == map.end()) {
            return false;
        }
        auto removed = std::move(item)
        write_guard(removed);
        return map.erase(item);
    }

private:
    std::unordered_map<K, std::unique_ptr<Item<V>>> map;
    std::shared_mutex mutex;

    struct Item
    {
        std::shared_mutex mutex;
        V value;
    };

    class read_guard
    {
        std::shared_lock lock;
        Item *item;

        read_guard(Item *item) : lock(item->mutex), item(item)
        {
        }

        V &operator*() { return item->value; }
        V *operator->() { return &item->value; }
    };

    class write_guard
    {
        std::unique_lock lock;
        Item *item;

        write_guard(Item *item) : lock(item->mutex), item(item)
        {
        }

        V &operator*() { return item->value; }
        V *operator->() { return &item->value; }
    };
};