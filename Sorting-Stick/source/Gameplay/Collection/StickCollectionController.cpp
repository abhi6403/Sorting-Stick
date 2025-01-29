#include "Gameplay/Collection/StickCollectionController.h"
#include "Gameplay/Collection/StickCollectionView.h"
#include "Gameplay/Collection/StickCollectionModel.h"
#include "Gameplay/GameplayService.h"
#include "Global/ServiceLocator.h"
#include "Gameplay/Collection/Stick.h"
#include <random>
#include<vector>
using namespace std;

namespace Gameplay
{
	namespace Collection
	{
		using namespace UI::UIElement;
		using namespace Global;
		using namespace Graphics;
		using namespace Sound;

		StickCollectionController::StickCollectionController()
		{
			collection_view = new StickCollectionView();
			collection_model = new StickCollectionModel();

			for (int i = 0; i < collection_model->number_of_elements; i++) sticks.push_back(new Stick(i));
		}

		StickCollectionController::~StickCollectionController()
		{
			destroy();
		}

		void StickCollectionController::initialize()
		{
			sort_state = SortState::NOT_SORTING;
			collection_view->initialize(this);
			initializeSticks();
			reset();
		}

		void StickCollectionController::initializeSticks()
		{
			float rectangle_width = calculateStickWidth();


			for (int i = 0; i < collection_model->number_of_elements; i++)
			{
				float rectangle_height = calculateStickHeight(i); //calc height

				sf::Vector2f rectangle_size = sf::Vector2f(rectangle_width, rectangle_height);

				sticks[i]->stick_view->initialize(rectangle_size, sf::Vector2f(0, 0), 0, collection_model->element_color);
			}
		}

		void StickCollectionController::update()
		{
			processSortThreadState();
			collection_view->update();
			for (int i = 0; i < sticks.size(); i++) sticks[i]->stick_view->update();
		}

		void StickCollectionController::render()
		{
			collection_view->render();
			for (int i = 0; i < sticks.size(); i++) sticks[i]->stick_view->render();
		}

		float StickCollectionController::calculateStickWidth()
		{
			float total_space = static_cast<float>(ServiceLocator::getInstance()->getGraphicService()->getGameWindow()->getSize().x);

			// Calculate total spacing as 10% of the total space
			float total_spacing = collection_model->space_percentage * total_space;

			// Calculate the space between each stick
			float space_between = total_spacing / (collection_model->number_of_elements - 1);
			collection_model->setElementSpacing(space_between);

			// Calculate the remaining space for the rectangles
			float remaining_space = total_space - total_spacing;

			// Calculate the width of each rectangle
			float rectangle_width = remaining_space / collection_model->number_of_elements;

			return rectangle_width;
		}

		float StickCollectionController::calculateStickHeight(int array_pos)
		{
			return (static_cast<float>(array_pos + 1) / collection_model->number_of_elements) * collection_model->max_element_height;
		}

		void StickCollectionController::updateStickPosition()
		{
			for (int i = 0; i < sticks.size(); i++)
			{
				float x_position = (i * sticks[i]->stick_view->getSize().x) + ((i) * collection_model->elements_spacing);
				float y_position = collection_model->element_y_position - sticks[i]->stick_view->getSize().y;

				sticks[i]->stick_view->setPosition(sf::Vector2f(x_position, y_position));
			}
		}

		void StickCollectionController::updateStickPosition(int i)
		{
			float x_position = (i * sticks[i]->stick_view->getSize().x) + ((i)*collection_model->elements_spacing);
			float y_position = collection_model->element_y_position - sticks[i]->stick_view->getSize().y;

			sticks[i]->stick_view->setPosition(sf::Vector2f(x_position, y_position));
		}

		void StickCollectionController::shuffleSticks()
		{
			std::random_device device;
			std::mt19937 random_engine(device());

			std::shuffle(sticks.begin(), sticks.end(), random_engine);
			updateStickPosition();
		}

		bool StickCollectionController::compareSticksByData(const Stick* a, const Stick* b) const
		{
			return a->data < b->data;
		}

		void StickCollectionController::processSortThreadState()
		{
			if (sort_thread.joinable() && isCollectionSorted())
			{
				sort_thread.join();
				sort_state = SortState::NOT_SORTING;
			}

		}


		void StickCollectionController::resetSticksColor()
		{
			for (int i = 0; i < sticks.size(); i++) sticks[i]->stick_view->setFillColor(collection_model->element_color);
		}

		void StickCollectionController::resetVariables()
		{
			number_of_comparisons = 0;
			number_of_array_access = 0;
		}

		void StickCollectionController::reset()
		{
			current_operation_delay = 0;
			color_delay = 0;
			sort_state = SortState::NOT_SORTING;
			if (sort_thread.joinable())
			{
				sort_thread.join();
			}

			shuffleSticks();
			resetSticksColor();
			resetVariables();
		}

		void StickCollectionController::sortElements(SortType sort_type)
		{
			current_operation_delay = collection_model->operation_delay;
			color_delay = collection_model->initial_color_delay;
			this->sort_type = sort_type;
			sort_state = SortState::SORTING;
			
			switch (sort_type)
			{
			case Gameplay::Collection::SortType::BUBBLE_SORT:
				time_complexity = "O(n^2)";
				sort_thread = std::thread(&StickCollectionController::processBubbleSort, this);
				break;
			case Gameplay::Collection::SortType::INSERTION_SORT:
				time_complexity = "O(n^2)";
				sort_thread = thread(&StickCollectionController::processInsertionSort, this);
				break;
			case Gameplay::Collection::SortType::SELECTION_SORT:
				time_complexity = "O(n^2)";
				sort_thread = thread(&StickCollectionController::processSelectionSort, this);
				break;
			case Gameplay::Collection::SortType::MERGE_SORT:
				time_complexity = "O(n log n)";
				sort_thread = thread(&StickCollectionController::processMergeSort, this);
				break;
			case Gameplay::Collection::SortType::QUICK_SORT:
				time_complexity = "O(n log n)";
				sort_thread = thread(&StickCollectionController::processQuickSort, this);
				break;
			case Gameplay::Collection::SortType::RADIX_SORT:
				time_complexity = "O(n log n)";
				sort_thread = thread(&StickCollectionController::processRadixSort, this);
				break;
			}
		}

		void StickCollectionController::processBubbleSort()
		{
			SoundService* sound_service = ServiceLocator::getInstance()->getSoundService();

			for (int i = 0; i < sticks.size(); i++)
			{
				if (sort_state == SortState::NOT_SORTING)
				{
					break;
				}

				bool swapped = false;

				for (int j = 0; j < sticks.size() - i; j++)
				{
					if (sort_state == SortState::NOT_SORTING)
					{
						break;
					}
					
					number_of_array_access += 2;
					number_of_comparisons++;
					sound_service->playSound(SoundType::COMPARE_SFX);

					sticks[j]->stick_view->setFillColor(collection_model->processing_element_color);
					sticks[j]->stick_view->setFillColor(collection_model->processing_element_color);

					if (sticks[j - 1]->data > sticks[j]->data)
					{
						std::swap(sticks[j - 1], sticks[j]);
						swapped = true;
					}

					this_thread::sleep_for(chrono::milliseconds(current_operation_delay));

					sticks[j - 1]->stick_view->setFillColor(collection_model->element_color);
					sticks[j]->stick_view->setFillColor(collection_model->element_color);

					updateStickPosition();
				}

				if (sticks.size() - i - 1 >= 0)
				{
					sticks[sticks.size() - 1 - 1]->stick_view->setFillColor(collection_model->placement_position_element_color);
				}

				if (!swapped)
				{
					break;
				}
			}
			setCompleteColor();
		}

		void StickCollectionController::processInsertionSort()
		{
			SoundService* sound_service = ServiceLocator::getInstance()->getSoundService();

			for (int i = 1; i < sticks.size(); i++)
			{
				if (sort_state == SortState::NOT_SORTING)
				{
					break;
				}

				int j = i - 1;
				Stick* key = sticks[i];

				number_of_array_access++;

				key->stick_view->setFillColor(collection_model->processing_element_color);

				this_thread::sleep_for(chrono::milliseconds(current_operation_delay));

				while (j >= 0 && sticks[j]->data > key->data)
				{
					if (sort_state == SortState::NOT_SORTING)
					{
						break;
					}

					number_of_comparisons++;
					number_of_array_access++;

					sticks[j + 1] = sticks[j];
					number_of_array_access++;

					sticks[j + 1]->stick_view->setFillColor(collection_model->processing_element_color);
					j--;
					sound_service->playSound(SoundType::COMPARE_SFX);
					updateStickPosition();

					this_thread::sleep_for(chrono::milliseconds(current_operation_delay));
					sticks[j + 2]->stick_view->setFillColor(collection_model->selected_element_color);
					
				}

				sticks[j + 1] = key;
				number_of_array_access++;
				sticks[j = 1]->stick_view->setFillColor(collection_model->temporary_processing_color);
				sound_service->playSound(SoundType::COMPARE_SFX);
				updateStickPosition();
				this_thread::sleep_for(chrono::milliseconds(current_operation_delay));
				sticks[j + 1]->stick_view->setFillColor(collection_model->selected_element_color);

			}
			setCompleteColor();
			
		}

		void StickCollectionController::processSelectionSort()
		{
			SoundService* sound_service = ServiceLocator::getInstance()->getSoundService();

			for (int i = 0; i < sticks.size() - 1; ++i)
			{
				if (sort_state == SortState::NOT_SORTING)
				{
					break;
				}

				int min_index = i;
				sticks[i]->stick_view->setFillColor(collection_model->selected_element_color);

				for (int j = i + 1; j < sticks.size(); j++)
				{
					if (sort_state == SortState::NOT_SORTING)
					{
						break;
					}

					number_of_array_access++;
					number_of_comparisons++;

					sound_service->playSound(SoundType::COMPARE_SFX);
					sticks[j]->stick_view->setFillColor(collection_model->processing_element_color);
					this_thread::sleep_for(chrono::milliseconds(current_operation_delay));

					if (sticks[j]->data < sticks[min_index]->data)
					{
						sticks[min_index]->stick_view->setFillColor(collection_model->element_color);
						min_index = j;
						sticks[min_index]->stick_view->setFillColor(collection_model->temporary_processing_color);
					}
					else
					{
						sticks[j]->stick_view->setFillColor(collection_model->element_color);
					}
				}

				swap(sticks[min_index], sticks[i]);
				number_of_array_access++;
				sticks[i]->stick_view->setFillColor(collection_model->placement_position_element_color);
				updateStickPosition();
			}

			sticks[sticks.size() - 1]->stick_view->setFillColor(collection_model->placement_position_element_color);

			setCompleteColor();
		}

		void StickCollectionController::processRadixSort()
		{
			radixSort();
			setCompleteColor();
		}

		void StickCollectionController::countSort(int exponent)
		{
			SoundService* sound_service = ServiceLocator::getInstance()->getSoundService();

			vector<Stick*>output(sticks.size());
			vector<int>count(10, 0);

			for (int i = 0; i < sticks.size(); i++)
			{
				sound_service->playSound(SoundType::COMPARE_SFX);
				int digit = (sticks[i]->data / exponent) % 10;
				count[digit]++;
				number_of_array_access++;
				sticks[i]->stick_view->setFillColor(collection_model->processing_element_color);
				this_thread::sleep_for(chrono::milliseconds(current_operation_delay / 2));
				sticks[i]->stick_view->setFillColor(collection_model->element_color);
			}

			for (int i = 1; i < 10; i++)
			{
				count[i] += count[i - 1];
			}

			for (int i = sticks.size() - 1; i >= 0; --i)
			{
				int digit = (sticks[i]->data / exponent) % 10;
				output[count[digit] - 1] = sticks[i];
				output[count[digit] - 1]->stick_view->setFillColor(collection_model->temporary_processing_color);
				count[digit]--;
				number_of_array_access++;
			}

			for (int i = 0; i < sticks.size(); i++)
			{
				sticks[i] = output[i];
				sticks[i]->stick_view->setFillColor(collection_model->placement_position_element_color);
				updateStickPosition(i);
				this_thread::sleep_for(chrono::milliseconds(current_operation_delay));
			}			
			
		}

		void StickCollectionController::radixSort()
		{
			int maxElement = INT_MIN;
			const int size = sticks.size();

			for (int i = 0; i < size; ++i)
			{
				maxElement = max(sticks[i]->data, maxElement);
			}

			for (int exponent = 1; maxElement / exponent > 0; exponent *= 10)
			{
				countSort(exponent);
			}
		}

		void StickCollectionController::processQuickSort()
		{
			quickSort(0, sticks.size() - 1);
			setCompleteColor();
		}

		int StickCollectionController::partition(int left, int right)
		{
			SoundService* sound_service = ServiceLocator::getInstance()->getSoundService();

			sticks[right]->stick_view->setFillColor(collection_model->selected_element_color);
			int i = left - 1;

			for (int j = left; j < right; j++)
			{
				sticks[j]->stick_view->setFillColor(collection_model->processing_element_color);

				number_of_array_access += 2;
				number_of_comparisons++;

				if (sticks[j]->data < sticks[right]->data)
				{
					i++;
					swap(sticks[i], sticks[j]);
					number_of_array_access += 3;
					sound_service->playSound(SoundType::COMPARE_SFX);

					updateStickPosition();

					this_thread::sleep_for(chrono::milliseconds(current_operation_delay));
				}

				sticks[j]->stick_view->setFillColor(collection_model->element_color);
			}
			swap(sticks[i + 1], sticks[right]);
			number_of_array_access += 3;
			updateStickPosition();

			return i + 1;
		}

		void StickCollectionController::quickSort(int left, int right)
		{
			if (left < right)
			{
				int pivot_index = partition(left, right);

				quickSort(left, pivot_index - 1);
				quickSort(pivot_index + 1, right);

				for (int i = left; i <= right; i++)
				{
					sticks[i]->stick_view->setFillColor(collection_model->placement_position_element_color);
					updateStickPosition();
				}
			}
		}

		void StickCollectionController::processMergeSort()
		{
			mergeSort(0, sticks.size() - 1);
			setCompleteColor();
		}

		void StickCollectionController::merge(int left, int mid, int right)
		{
			SoundService* sound_service = ServiceLocator::getInstance()->getSoundService();

			vector<Stick*>temp(right - left + 1);
			int k = 0;

			for (int index = left; index <= right; ++index)
			{
				temp[k++] = sticks[index];
				number_of_array_access++;
				sticks[index]->stick_view->setFillColor(collection_model->temporary_processing_color);
				updateStickPosition();
			}

			int i = 0;
			int j = mid - left + 1;
			k = left;

			while (i < mid - left + 1 && j < temp.size())
			{
				number_of_comparisons++;
				number_of_array_access += 2;
				if (temp[i]->data <= temp[j]->data)
				{
					sticks[k] = temp[i++];
					number_of_array_access++;
				}
				else
				{
					sticks[k] = temp[j++];
					number_of_array_access++;
				}

				sound_service->playSound(SoundType::COMPARE_SFX);
				sticks[k]->stick_view->setFillColor(collection_model->processing_element_color);
				updateStickPosition();

				this_thread::sleep_for(chrono::milliseconds(current_operation_delay));
				k++;
			}

			while (i < mid - left + 1 || j < temp.size())
			{
				number_of_array_access++;
				if (i < mid - left + 1)
				{
					sticks[k] = temp[i++];
				}
				else
				{
					sticks[k] = temp[j++];
				}

				sound_service->playSound(SoundType::COMPARE_SFX);
				sticks[k]->stick_view->setFillColor(collection_model->processing_element_color);
				updateStickPosition();
				this_thread::sleep_for(chrono::milliseconds(current_operation_delay));
				k++;
			}
		}

		void StickCollectionController::mergeSort(int left, int right)
		{
			if (left < right)
			{
				int mid = left + (right - left) / 2;

				mergeSort(left, mid);
				mergeSort(mid + 1, right);

				merge(left,mid,right);
			}
		}

		void StickCollectionController::processInPlaceMergeSort()
		{
			inPlaceMergeSort(0, sticks.size() - 1);
			setCompleteColor();
		}

		void StickCollectionController::inPlaceMerge(int left, int mid, int right)
		{
			SoundService* sound_service = ServiceLocator::getInstance()->getSoundService();

			int start2 = mid + 1;

			if (sticks[mid]->data <= sticks[start2]->data)
			{
				number_of_comparisons++;
				number_of_array_access += 2;
				return;
			}

			while (left <= mid && start2 <= right)
			{
				number_of_comparisons++;
				number_of_array_access += 2;

				if (sticks[left]->data <= sticks[start2]->data)
				{
					left++;
				}
				else
				{
					Stick* temp = sticks[start2];
					int index = start2;

					while (index != left)
					{
						sticks[index] = sticks[index - 1];
						index--;
						number_of_array_access += 2;
					}

					sticks[left] = temp;
					number_of_array_access++;

					left++;
					mid++;
					start2++;
					updateStickPosition();
				}
				sound_service->playSound(SoundType::COMPARE_SFX);
				sticks[left - 1]->stick_view->setFillColor(collection_model->processing_element_color);
				this_thread::sleep_for(chrono::milliseconds(current_operation_delay));
				sticks[left - 1]->stick_view->setFillColor(collection_model->element_color);
			}

		}

		void StickCollectionController::inPlaceMergeSort(int left, int right)
		{
			if (left < right)
			{
				int mid = left + (right - left) / 2;

				inPlaceMergeSort(left, mid);
				inPlaceMergeSort(mid + 1, right);

				inPlaceMerge(left, mid, right);
			}
		}

		void StickCollectionController::setCompleteColor()
		{
			SoundService* sound_service = ServiceLocator::getInstance()->getSoundService();

			for (int k = 0; k < sticks.size(); k++)
			{
				if (sort_state == SortState::NOT_SORTING)
				{
					break;
				}

				sticks[k]->stick_view->setFillColor(collection_model->element_color);
			}

			for (int i = 0; i < sticks.size(); i++)
			{
				if (sort_state == SortState::NOT_SORTING)
				{
					break;
				}

				sound_service->playSound(SoundType::COMPARE_SFX);

				sticks[i]->stick_view->setFillColor(collection_model->placement_position_element_color);
				this_thread::sleep_for(chrono::milliseconds(color_delay));
			}

			if (sort_state == SortState::SORTING)
			{
				sound_service->playSound(SoundType::SCREAM);
			}
		}

		bool StickCollectionController::isCollectionSorted()
		{
			for (int i = 1; i < sticks.size(); i++) if (sticks[i] < sticks[i - 1]) return false;
			return true;
		}

		void StickCollectionController::destroy()
		{
			current_operation_delay = 0;
			if (sort_thread.joinable()) sort_thread.join();

			for (int i = 0; i < sticks.size(); i++) delete(sticks[i]);
			sticks.clear();

			delete (collection_view);
			delete (collection_model);
		}

		SortType StickCollectionController::getSortType() { return sort_type; }

		int StickCollectionController::getNumberOfComparisons() { return number_of_comparisons; }

		int StickCollectionController::getNumberOfArrayAccess() { return number_of_array_access; }

		int StickCollectionController::getNumberOfSticks() { return collection_model->number_of_elements; }

		int StickCollectionController::getDelayMilliseconds() { return current_operation_delay; }

		sf::String StickCollectionController::getTimeComplexity() { return time_complexity; }
	}
}


