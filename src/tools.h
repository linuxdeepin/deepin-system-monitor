template<class T>
void resizeList(QList<T> & list, int newSize) {
    int diff = newSize - list.size();
    T t;
    if (diff > 0) {
        list.reserve(diff);
        while (diff--) list.append(t);
    } else if (diff < 0) list.erase(list.end() + diff, list.end());
}
