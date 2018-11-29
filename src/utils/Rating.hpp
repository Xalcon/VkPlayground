#pragma once
#include <vector>
#include <functional>

template <typename TElement>
struct Rating
{
	TElement element;
	float score;
	int index;
};

template <typename TElement>
Rating<TElement> GetBestRatedElement(std::vector<TElement> elements, std::function<float(TElement)> ratingCallback)
{
	std::vector<Rating<TElement>> list;
	int i = 0;
	for (auto& e : elements)
		list.emplace_back(Rating<TElement> { e, ratingCallback(e), i++ });

	return *std::max_element(list.begin(), list.end(), [](auto a, auto b) { return a.score > b.score; });
}

template <typename TElement>
Rating<TElement> GetBestRatedElement(std::vector<TElement> elements, std::function<float(TElement, int)> ratingCallback)
{
	std::vector<Rating<TElement>> list;
	int i = 0;
	for (auto& e : elements)
		list.emplace_back(Rating<TElement> { e, ratingCallback(e, i), i++ });

	return *std::max_element(list.begin(), list.end(), [](auto a, auto b) { return a.score > b.score; });
}