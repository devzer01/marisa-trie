#include <cassert>
#include <sstream>

#include <marisa/trie.h>

namespace {

class FindCallback {
 public:
  FindCallback(std::vector<marisa::UInt32> *key_ids,
      std::vector<std::size_t> *key_lengths)
      : key_ids_(key_ids), key_lengths_(key_lengths) {}
  FindCallback(const FindCallback &callback)
      : key_ids_(callback.key_ids_), key_lengths_(callback.key_lengths_) {}

  bool operator()(marisa::UInt32 key_id, std::size_t key_length) const {
    key_ids_->push_back(key_id);
    key_lengths_->push_back(key_length);
    return true;
  }

 private:
  std::vector<marisa::UInt32> *key_ids_;
  std::vector<std::size_t> *key_lengths_;

  // Disallows assignment.
  FindCallback &operator=(const FindCallback &);
};

class PredictCallback {
 public:
  PredictCallback(std::vector<marisa::UInt32> *key_ids,
      std::vector<std::string> *keys)
      : key_ids_(key_ids), keys_(keys) {}
  PredictCallback(const PredictCallback &callback)
      : key_ids_(callback.key_ids_), keys_(callback.keys_) {}

  bool operator()(marisa::UInt32 key_id, const char *key,
      std::size_t key_length) const {
    key_ids_->push_back(key_id);
    keys_->push_back(std::string(key, key_length));
    return true;
  }

 private:
  std::vector<marisa::UInt32> *key_ids_;
  std::vector<std::string> *keys_;

  // Disallows assignment.
  PredictCallback &operator=(const PredictCallback &);
};

void TestTrie() {
  marisa::Trie trie;
  assert(trie.num_keys() == 0);
  assert(trie.num_tries() == 0);
  assert(trie.num_nodes() == 0);
  assert(trie.total_size() == (sizeof(marisa::UInt32) * 22));

  std::vector<std::string> keys;
  trie.build(keys);
  assert(trie.num_keys() == 0);
  assert(trie.num_tries() == 1);
  assert(trie.num_nodes() == 1);

  keys.push_back("apple");
  keys.push_back("and");
  keys.push_back("Bad");
  keys.push_back("apple");
  keys.push_back("app");

  std::vector<marisa::UInt32> key_ids;
  trie.build(keys, &key_ids, 1 | MARISA_WITHOUT_TAIL | MARISA_LABEL_ORDER);
  assert(trie.num_keys() == 4);
  assert(trie.num_tries() == 1);
  assert(trie.num_nodes() == 11);

  assert(key_ids.size() == 5);
  assert(key_ids[0] == 3);
  assert(key_ids[1] == 1);
  assert(key_ids[2] == 0);
  assert(key_ids[3] == 3);
  assert(key_ids[4] == 2);

  char key_buf[256];
  std::size_t key_length;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(keys[i] == key_buf);
  }

  trie.clear();
  assert(trie.num_keys() == 0);
  assert(trie.num_tries() == 0);
  assert(trie.num_nodes() == 0);
  assert(trie.total_size() == (sizeof(marisa::UInt32) * 22));

  trie.build(keys, &key_ids, 1 | MARISA_WITHOUT_TAIL | MARISA_WEIGHT_ORDER);
  assert(trie.num_keys() == 4);
  assert(trie.num_tries() == 1);
  assert(trie.num_nodes() == 11);

  assert(key_ids.size() == 5);
  assert(key_ids[0] == 3);
  assert(key_ids[1] == 1);
  assert(key_ids[2] == 2);
  assert(key_ids[3] == 3);
  assert(key_ids[4] == 0);

  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);
  }

  assert(trie["appl"] == trie.notfound());
  assert(trie["Apple"] == trie.notfound());
  assert(trie["applex"] == trie.notfound());

  assert(trie.find_first("ap") == trie.notfound());
  assert(trie.find_first("applex") == trie["app"]);

  assert(trie.find_last("ap") == trie.notfound());
  assert(trie.find_last("applex") == trie["apple"]);

  std::vector<marisa::UInt32> ids;
  assert(trie.find("ap", &ids) == 0);
  assert(trie.find("applex", &ids) == 2);
  assert(ids.size() == 2);
  assert(ids[0] == trie["app"]);
  assert(ids[1] == trie["apple"]);

  std::vector<std::size_t> lengths;
  assert(trie.find("Baddie", &ids, &lengths) == 1);
  assert(ids.size() == 3);
  assert(ids[0] == trie["app"]);
  assert(ids[1] == trie["apple"]);
  assert(ids[2] == trie["Bad"]);
  assert(lengths.size() == 1);
  assert(lengths[0] == 3);

  ids.clear();
  lengths.clear();
  assert(trie.find_callback("anderson", FindCallback(&ids, &lengths)) == 1);
  assert(ids.size() == 1);
  assert(ids[0] == trie["and"]);
  assert(lengths.size() == 1);
  assert(lengths[0] == 3);

  assert(trie.predict("") == 4);
  assert(trie.predict("a") == 3);
  assert(trie.predict("ap") == 2);
  assert(trie.predict("app") == 2);
  assert(trie.predict("appl") == 1);
  assert(trie.predict("apple") == 1);
  assert(trie.predict("appleX") == 0);
  assert(trie.predict("an") == 1);
  assert(trie.predict("and") == 1);
  assert(trie.predict("andX") == 0);
  assert(trie.predict("B") == 1);
  assert(trie.predict("BX") == 0);
  assert(trie.predict("X") == 0);

  ids.clear();
  assert(trie.predict("a", &ids) == 3);
  assert(ids.size() == 3);
  assert(ids[0] == trie["app"]);
  assert(ids[1] == trie["and"]);
  assert(ids[2] == trie["apple"]);

  std::vector<std::string> strs;
  assert(trie.predict("a", &ids, &strs) == 3);
  assert(ids.size() == 6);
  assert(ids[3] == trie["app"]);
  assert(ids[4] == trie["apple"]);
  assert(ids[5] == trie["and"]);
  assert(strs[0] == "app");
  assert(strs[1] == "apple");
  assert(strs[2] == "and");
}

void TestPrefixTrie() {
  std::vector<std::string> keys;
  keys.push_back("after");
  keys.push_back("bar");
  keys.push_back("car");
  keys.push_back("caster");

  marisa::Trie trie;
  std::vector<marisa::UInt32> key_ids;
  trie.build(keys, &key_ids, 1 | MARISA_PREFIX_TRIE
      | MARISA_TEXT_TAIL | MARISA_LABEL_ORDER);
  assert(trie.num_keys() == 4);
  assert(trie.num_tries() == 1);
  assert(trie.num_nodes() == 7);

  char key_buf[256];
  std::size_t key_length;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(key_ids[i] == i);
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(keys[i] == key_buf);
  }

  trie.restore(key_ids[0], NULL, 0, &key_length);
  assert(key_length == keys[0].length());
  try {
    trie.restore(key_ids[0], NULL, 5, &key_length);
    assert(false);
  } catch (const marisa::Exception &ex) {
    assert(ex.status() == MARISA_PARAM_ERROR);
  }
  trie.restore(key_ids[0], key_buf, 5, &key_length);
  assert(key_length == keys[0].length());
  trie.restore(key_ids[0], key_buf, 6, &key_length);
  assert(key_length == keys[0].length());

  trie.build(keys, &key_ids, 2 | MARISA_PREFIX_TRIE
      | MARISA_WITHOUT_TAIL | MARISA_WEIGHT_ORDER);
  assert(trie.num_keys() == 4);
  assert(trie.num_tries() == 2);
  assert(trie.num_nodes() == 16);

  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(key_ids[i] == i);
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(keys[i] == key_buf);
  }

  trie.restore(key_ids[0], NULL, 0, &key_length);
  assert(key_length == keys[0].length());
  try {
    trie.restore(key_ids[0], NULL, 5, &key_length);
    assert(false);
  } catch (const marisa::Exception &ex) {
    assert(ex.status() == MARISA_PARAM_ERROR);
  }
  trie.restore(key_ids[0], key_buf, 5, &key_length);
  assert(key_length == keys[0].length());
  trie.restore(key_ids[0], key_buf, 6, &key_length);
  assert(key_length == keys[0].length());

  trie.build(keys, &key_ids, 2 | MARISA_PREFIX_TRIE
      | MARISA_TEXT_TAIL | MARISA_LABEL_ORDER);
  assert(trie.num_keys() == 4);
  assert(trie.num_tries() == 2);
  assert(trie.num_nodes() == 14);

  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(key_ids[i] == i);
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(keys[i] == key_buf);
  }

  trie.save("trie-test.dat");

  trie.clear();
  marisa::Mapper mapper;
  trie.mmap(&mapper, "trie-test.dat");
  assert(trie.num_keys() == 4);
  assert(trie.num_tries() == 2);
  assert(trie.num_nodes() == 14);

  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(key_ids[i] == i);
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(keys[i] == key_buf);
  }

  std::stringstream stream;
  trie.write(stream);

  trie.clear();
  trie.read(stream);
  assert(trie.num_keys() == 4);
  assert(trie.num_tries() == 2);
  assert(trie.num_nodes() == 14);

  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(key_ids[i] == i);
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(keys[i] == key_buf);
  }

  trie.build(keys, &key_ids, 3 | MARISA_PREFIX_TRIE
      | MARISA_WITHOUT_TAIL | MARISA_WEIGHT_ORDER);
  assert(trie.num_keys() == 4);
  assert(trie.num_tries() == 3);
  assert(trie.num_nodes() == 19);

  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(key_ids[i] == i);
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(keys[i] == key_buf);
  }

  assert(trie["ca"] == trie.notfound());
  assert(trie["card"] == trie.notfound());

  std::size_t length = 0;
  assert(trie.find_first("ca") == trie.notfound());
  assert(trie.find_first("car") == trie["car"]);
  assert(trie.find_first("card", &length) == trie["car"]);
  assert(length == 3);

  assert(trie.find_last("afte") == trie.notfound());
  assert(trie.find_last("after") == trie["after"]);
  assert(trie.find_last("afternoon", &length) == trie["after"]);
  assert(length == 5);

  std::vector<marisa::UInt32> ids;
  assert(trie.predict("ca", &ids) == 2);
  assert(ids.size() == 2);
  assert(ids[0] == trie["car"]);
  assert(ids[1] == trie["caster"]);

  assert(trie.predict("ca", &ids, NULL, 1) == 1);
  assert(ids.size() == 3);
  assert(ids[2] == trie["car"]);

  ids.clear();
  std::vector<std::string> strs;
  assert(trie.predict("ca", &ids, &strs, 1) == 1);
  assert(ids.size() == 1);
  assert(ids[0] == trie["car"]);
  assert(strs[0] == "car");

  strs.clear();
  assert(trie.predict_callback("", PredictCallback(&ids, &strs)) == 4);
  assert(ids.size() == 5);
  assert(ids[1] == trie["car"]);
  assert(ids[2] == trie["caster"]);
  assert(ids[3] == trie["after"]);
  assert(ids[4] == trie["bar"]);
  assert(strs[0] == "car");
  assert(strs[1] == "caster");
  assert(strs[2] == "after");
  assert(strs[3] == "bar");
}

void TestPatriciaTrie() {
  std::vector<std::string> keys;
  keys.push_back("bach");
  keys.push_back("bet");
  keys.push_back("chat");
  keys.push_back("check");
  keys.push_back("check");

  marisa::Trie trie;
  std::vector<marisa::UInt32> key_ids;
  trie.build(keys, &key_ids, 1);
  assert(trie.num_keys() == 4);
  assert(trie.num_tries() == 1);
  assert(trie.num_nodes() == 7);

  assert(key_ids.size() == 5);
  assert(key_ids[0] == 2);
  assert(key_ids[1] == 3);
  assert(key_ids[2] == 1);
  assert(key_ids[3] == 0);
  assert(key_ids[4] == 0);

  char key_buf[256];
  std::size_t key_length;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(keys[i] == key_buf);
  }

  trie.build(keys, &key_ids, 2 | MARISA_WITHOUT_TAIL);
  assert(trie.num_keys() == 4);
  assert(trie.num_tries() == 2);
  assert(trie.num_nodes() == 17);

  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(keys[i] == key_buf);
  }

  trie.build(keys, &key_ids, 2);
  assert(trie.num_keys() == 4);
  assert(trie.num_tries() == 2);
  assert(trie.num_nodes() == 14);

  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(keys[i] == key_buf);
  }

  trie.build(keys, &key_ids, 3 | MARISA_WITHOUT_TAIL);
  assert(trie.num_keys() == 4);
  assert(trie.num_tries() == 3);
  assert(trie.num_nodes() == 20);

  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(keys[i] == key_buf);
  }

  std::stringstream stream;
  trie.write(stream);
  trie.clear();

  trie.read(stream);
  assert(trie.num_keys() == 4);
  assert(trie.num_tries() == 3);
  assert(trie.num_nodes() == 20);

  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(keys[i] == key_buf);
  }
}

void TestEmptyString() {
  std::vector<std::string> keys;
  keys.push_back("");

  marisa::Trie trie;
  std::vector<marisa::UInt32> key_ids;
  trie.build(keys, &key_ids);
  assert(trie.num_keys() == 1);
  assert(trie.num_tries() == 1);
  assert(trie.num_nodes() == 1);

  assert(key_ids.size() == 1);
  assert(key_ids[0] == 0);

  assert(trie[""] == 0);
  assert(trie[0U] == "");

  assert(trie["x"] == trie.notfound());
  assert(trie.find_first("") == 0);
  assert(trie.find_first("x") == 0);
  assert(trie.find_last("") == 0);
  assert(trie.find_last("x") == 0);

  std::vector<marisa::UInt32> ids;
  assert(trie.find("xyz", &ids) == 1);
  assert(ids.size() == 1);
  assert(ids[0] == trie[""]);

  std::vector<std::size_t> lengths;
  assert(trie.find("xyz", &ids, &lengths) == 1);
  assert(ids.size() == 2);
  assert(ids[0] == trie[""]);
  assert(ids[1] == trie[""]);
  assert(lengths.size() == 1);
  assert(lengths[0] == 0);

  ids.clear();
  lengths.clear();
  assert(trie.find_callback("xyz", FindCallback(&ids, &lengths)) == 1);
  assert(ids.size() == 1);
  assert(ids[0] == trie[""]);
  assert(lengths.size() == 1);
  assert(lengths[0] == 0);

  ids.clear();
  assert(trie.predict("xyz", &ids) == 0);

  assert(trie.predict("", &ids) == 1);
  assert(ids.size() == 1);
  assert(ids[0] == trie[""]);

  std::vector<std::string> strs;
  assert(trie.predict("", &ids, &strs) == 1);
  assert(ids.size() == 2);
  assert(ids[1] == trie[""]);
  assert(strs[0] == "");
}

void TestBinaryKey() {
  std::string binary_key = "NP";
  binary_key += '\0';
  binary_key += "Trie";

  std::vector<std::string> keys;
  keys.push_back(binary_key);

  marisa::Trie trie;
  std::vector<marisa::UInt32> key_ids;
  trie.build(keys, &key_ids, 1 | MARISA_WITHOUT_TAIL);
  assert(trie.num_keys() == 1);
  assert(trie.num_tries() == 1);
  assert(trie.num_nodes() == 8);

  assert(key_ids.size() == 1);
  char key_buf[256];
  std::size_t key_length;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(key_ids[i] == i);
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(std::string(key_buf, key_length) == keys[i]);
  }

  trie.build(keys, &key_ids, 1 | MARISA_PREFIX_TRIE | MARISA_BINARY_TAIL);
  assert(trie.num_keys() == 1);
  assert(trie.num_tries() == 1);
  assert(trie.num_nodes() == 2);

  assert(key_ids.size() == 1);
  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(key_ids[i] == i);
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(std::string(key_buf, key_length) == keys[i]);
  }

  trie.build(keys, &key_ids, 1 | MARISA_PREFIX_TRIE | MARISA_TEXT_TAIL);
  assert(trie.num_keys() == 1);
  assert(trie.num_tries() == 1);
  assert(trie.num_nodes() == 2);

  assert(key_ids.size() == 1);
  for (std::size_t i = 0; i < keys.size(); ++i) {
    assert(key_ids[i] == i);
    assert(trie[keys[i]] == key_ids[i]);
    assert(trie[key_ids[i]] == keys[i]);

    trie.restore(key_ids[i], key_buf,
        (marisa::UInt32)sizeof(key_buf), &key_length);
    assert(key_length == keys[i].length());
    assert(std::string(key_buf, key_length) == keys[i]);
  }

  std::vector<marisa::UInt32> ids;
  assert(trie.predict_breadth_first("", &ids) == 1);
  assert(ids[0] == key_ids[0]);

  ids.clear();
  std::vector<std::string> strs;
  assert(trie.predict_depth_first("NP", &ids, &strs) == 1);
  assert(ids[0] == key_ids[0]);
  assert(strs[0] == keys[0]);
}

}  // namespace

int main() {
  TestTrie();
  TestPrefixTrie();
  TestPatriciaTrie();
  TestEmptyString();
  TestBinaryKey();

  return 0;
}
