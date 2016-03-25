#pragma once
#include "family_type.h"
#include "family_mask.h"
#include "entity_id.h"

namespace Halley {
	class Entity;

	class Family {
		friend class World;

	public:
		Family(FamilyMaskType mask);
		virtual ~Family() = default;

		size_t count() const
		{
			return elemCount;
		}

		void* getElement(size_t n) const
		{
			return static_cast<char*>(elems) + (n * elemSize);
		}

	protected:
		virtual void addEntity(Entity& entity) = 0;
		void removeEntity(Entity& entity);
		virtual void removeDeadEntities() = 0;
		
		void* elems;
		size_t elemCount;
		size_t elemSize;
		std::vector<EntityId> toRemove;

	private:
		FamilyMaskType inclusionMask;
	};

	template <typename T>
	class FamilyImpl : public Family
	{
		struct StorageType
		{
			EntityId entityId;
			union {
				std::array<char, sizeof(T) - std::max(sizeof(EntityId), sizeof(void*))> data;
				void* alignDummy;
			};
		};

	public:
		FamilyImpl() : Family(T::Type::readMask) {}
		
	protected:
		void addEntity(Entity& entity) override
		{
			entities.push_back(StorageType());
			auto& e = entities.back();
			e.entityId = entity.getUID();
			T::Type::loadComponents(entity, &e.data[0]);

			updateElems();
		}

		void removeDeadEntities() override
		{
			// Performance-critical code
			// Benchmarks suggest that using a std::vector is faster than std::set and std::unordered_set
			if (!toRemove.empty()) {
				std::sort(toRemove.begin(), toRemove.end());

				size_t size = entities.size();
				for (size_t i = 0; i < size; i++) {
					EntityId id = entities[i].entityId;
					auto iter = std::lower_bound(toRemove.begin(), toRemove.end(), id);
					if (iter != toRemove.end() && id == *iter) {
						toRemove.erase(iter);
						std::swap(entities[i], entities[size - 1]);
						entities.pop_back();
						size--;
						i--;
						if (toRemove.empty()) break;
					}
				}
				toRemove.clear();
			}
			updateElems();
		}

	private:
		std::vector<StorageType> entities;

		void updateElems()
		{
			elems = entities.data();
			elemCount = entities.size();
			elemSize = sizeof(StorageType);
		}
	};
}
