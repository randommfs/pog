#pragma once

#include <numeric>
#include <unordered_set>

#include <pog/item.h>
#include <pog/utils.h>

namespace pog {

template <typename ValueT>
class State
{
public:
	using ItemType = Item<ValueT>;
	using SymbolType = Symbol<ValueT>;

	State() : _index(std::numeric_limits<decltype(_index)>::max()) {}
	State(std::uint32_t index) : _index(index) {}

	std::uint32_t get_index() const { return _index; }
	void set_index(std::uint32_t index) { _index = index; }

	template <typename T>
	std::pair<ItemType*, bool> add_item(T&& item)
	{
		auto itr = std::lower_bound(_items.begin(), _items.end(), item, [](const auto& left, const auto& needle) {
			return *left.get() < needle;
		});

		if (itr == _items.end() || *itr->get() != item)
		{
			auto new_itr = _items.insert(itr, std::make_unique<ItemType>(std::forward<T>(item)));
			return {new_itr->get(), true};
		}
		else
			return {itr->get(), false};
	}

	std::size_t size() const { return _items.size(); }
	auto begin() const { return _items.begin(); }
	auto end() const { return _items.end(); }

	void add_transition(const SymbolType* symbol, const State* state)
	{
		_transitions.emplace(symbol, state);
	}

	void add_back_transition(const SymbolType* symbol, const State* state)
	{
		auto itr = _back_transitions.find(symbol);
		if (itr == _back_transitions.end())
		{
			_back_transitions.emplace(symbol, std::vector<const State*>{state});
			return;
		}

		auto state_itr = std::lower_bound(itr->second.begin(), itr->second.end(), state->get_index(), [](const auto& left, const auto& needle) {
			return left->get_index() < needle;
		});

		if (state_itr == itr->second.end() || (*state_itr)->get_index() != state->get_index())
			itr->second.insert(state_itr, state);
	}

	bool is_accepting() const
	{
		return std::count_if(_items.begin(), _items.end(), [](const auto& item) {
				return item->is_accepting();
			}) == 1;
	}

	std::string to_string(std::string_view arrow = "->", std::string_view eps = "<eps>", std::string_view sep = "<*>") const
	{
		std::vector<std::string> item_strings(_items.size());
		std::transform(_items.begin(), _items.end(), item_strings.begin(), [&](const auto& item) {
			return item->to_string(arrow, eps, sep);
		});
		return fmt::format("{}", fmt::join(item_strings.begin(), item_strings.end(), "\n"));
	}

	std::vector<const ItemType*> get_production_items() const
	{
		std::vector<const ItemType*> result;
		transform_if(_items.begin(), _items.end(), std::back_inserter(result),
			[](const auto& item) {
				return item->is_final();
			},
			[](const auto& item) {
				return item.get();
			}
		);
		return result;
	}

	std::unordered_map<const SymbolType*, std::vector<const ItemType*>> get_partitions() const
	{
		std::unordered_map<const SymbolType*, std::vector<const ItemType*>> result;

		for (const auto& item : _items)
		{
			if (item->is_final())
				continue;

			const auto* next_sym = item->get_read_symbol();
			if (auto itr = result.find(next_sym); itr == result.end())
				result.emplace(next_sym, std::vector<const ItemType*>{item.get()});
			else
				itr->second.push_back(item.get());
		}

		return result;
	}

	bool contains(const ItemType& item) const
	{
		return std::lower_bound(_items.begin(), _items.end(), item, [](const auto& left, const auto& needle) {
			return *left.get() < needle;
		}) != _items.end();
	}

	bool operator==(const State& rhs) const
	{
		return std::equal(_items.begin(), _items.end(), rhs._items.begin(), rhs._items.end(), [](const auto& left, const auto& right) {
			return *left.get() == *right.get();
		});
	}

	bool operator !=(const State& rhs) const
	{
		return !(*this == rhs);
	}

	const std::unordered_map<const SymbolType*, const State*>& get_transitions() const { return _transitions; }
	const std::unordered_map<const SymbolType*, std::vector<const State*>>& get_back_transitions() const { return _back_transitions; }

private:
	std::uint32_t _index;
	std::vector<std::unique_ptr<ItemType>> _items;
	std::unordered_map<const SymbolType*, const State*> _transitions;
	std::unordered_map<const SymbolType*, std::vector<const State*>> _back_transitions;
};

} // namespace pog
