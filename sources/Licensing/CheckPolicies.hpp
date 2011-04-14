/**************************************************************************
 *   Created: 2009/09/08 19:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: CheckPolicies.hpp 1072 2010-11-25 20:02:26Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__CheckPolicies_hpp__0909081926
#define INCLUDED_FILE__TUNNELEX__CheckPolicies_hpp__0909081926

#include "Types.hpp"

namespace TunnelEx { namespace Licensing {

	//////////////////////////////////////////////////////////////////////////

	template<typename ClientTrait, typename Value>
	struct ValueCheckPolicy {
		//...//
	};

	template<typename ClientTrait>
	struct ValueCheckPolicy<ClientTrait, size_t> {
		typedef size_t Value;
		inline static bool IsAvailable(Value keyVal, Value localVal) {
			return keyVal >= localVal;
		}
	};

	template<typename ClientTrait>
	struct ValueCheckPolicy<ClientTrait, bool> {
		typedef bool Value;
		inline static bool IsAvailable(Value keyVal, Value localVal) {
			return keyVal == localVal;
		}
	};


	//////////////////////////////////////////////////////////////////////////

	template<typename ClientTrait>
	struct CheckPolicy {
		
		typedef typename ClientTrait::Feature Feature;
		typedef typename Feature::Value FeatureValue;
		typedef typename ClientTrait::FeatureInfo FeatureInfo;
		typedef typename ClientTrait::LicenseKeyInfo LicenseKeyInfo;
		typedef typename ClientTrait::LocalInfo LocalInfo;
		typedef typename ClientTrait::WorkstationPropertiesQuery
			WorkstationPropertiesQuery;

		inline static bool CalculateScores(
					const WorkstationPropertyValues &control,
					const WorkstationPropertiesScores &scores,
					const WorkstationPropertyValues &local,
					Scores &result) {
			Scores resultTmp = 0;
			const WorkstationPropertyValues::const_iterator localEnd = local.end();
			const WorkstationPropertiesScores::const_iterator scoresEnd = scores.end();
			foreach (const WorkstationPropertyValues::value_type &controlI, control) {
				const WorkstationPropertyValues::const_iterator localPos
					= local.find(controlI.first);
				if (	localPos == localEnd
						||	!CheckWorkstationPropetyChange(controlI.second, localPos->second)) {
					const WorkstationPropertiesScores::const_iterator scoresPos
						= scores.find(controlI.first);
					if (scoresPos == scoresEnd) {
						return false;
					}
					resultTmp += scoresPos->second;
				}
			}
			result = resultTmp;
			return true;
		}

		inline static bool CheckWorkstationPropetiesChanges(
					const LicenseKeyInfo &keyInfo,
					const LocalInfo &localInfo) {
			return CheckWorkstationPropetiesChanges(
				keyInfo.limitations.workstationProperties,
				localInfo.workstationProperties);
		}

		inline static bool CheckWorkstationPropetiesChanges(
					const FeatureInfo &keyInfo,
					const LocalInfo &localInfo) {
			return CheckWorkstationPropetiesChanges(
				keyInfo.limitations.workstationProperties,
				localInfo.workstationProperties);
		}


		inline static bool CheckWorkstationPropetiesChanges(
					const WorkstationPropertyValues &control,
					const WorkstationPropertyValues &local) {
			const WorkstationPropertyValues::const_iterator end = local.end();
			foreach (const WorkstationPropertyValues::value_type &i, control) {
				const WorkstationPropertyValues::const_iterator pos
					= local.find(i.first);
				if (	pos == end
						|| !CheckWorkstationPropetyChange(i.second, pos->second)) {
					return false;
				}
			}
			return true;
		}
		
		inline static bool CheckWorkstationPropetyChange(
					const std::string &control,
					const std::string &local) {
			const size_t hashSize = 40;
			BOOST_ASSERT(!(control.size() % hashSize));
			BOOST_ASSERT(!(local.size() % hashSize));
			if (	!(control.size() % hashSize)
					&& !(local.size() % hashSize)
					&& !control.empty()) {
				const size_t subsCount = local.size() / hashSize;
				for (size_t i = 0; i < subsCount; ++i) {
					const std::string sub = std::string(
						local.begin() + (i * hashSize),
						local.begin() + ((i + 1) * hashSize));
					const boost::iterator_range<std::string::const_iterator> searchResult
						= boost::find_first(control, sub);
					if (	searchResult
							&& !(std::distance(control.begin(), searchResult.begin()) % hashSize)) {
						BOOST_ASSERT(!(std::distance(boost::end(searchResult), control.end()) % hashSize));
						return true;
					}
				}
			}
			return false;
		}

		inline static bool CheckTime(
					const TimeInterval &interval,
					const LocalInfo &localInfo) {
			return 
				(!interval.first || *interval.first <= localInfo.time)
				&&	(!interval.second || *interval.second >= localInfo.time);
		}

		inline static bool CheckTime(
					const Limitations &limitationsInfo,
					const LocalInfo &localInfo) {
			return CheckTime(limitationsInfo.timeInterval, localInfo);
		}

		inline static bool CheckTime(
					const LicenseKeyInfo &keyInfo,
					const LocalInfo &localInfo) {
			return
				CheckTime(keyInfo.limitations, localInfo)
				&& CheckTime(keyInfo.update, localInfo);
		}

		inline static UnactivityReason GetCheckTimeFailReason(
					const LicenseKeyInfo &keyInfo,
					const LocalInfo &localInfo) {
			return CheckTime(keyInfo, localInfo)
				?	UR_NO
				:	!CheckTime(keyInfo.update, localInfo)
					?	UR_UPDATE
					:	UR_TIME;
		}

		inline static bool CheckTime(
					const FeatureInfo &keyInfo,
					const LocalInfo &localInfo) {
			return CheckTime(keyInfo.limitations, localInfo);
		}

		inline static UnactivityReason GetCheckTimeFailReason(
					const FeatureInfo &keyInfo,
					const LocalInfo &localInfo) {
			return CheckTime(keyInfo, localInfo) ? UR_NO : UR_TIME;
		}

		inline static bool CheckScores(
					const LicenseKeyInfo &keyInfo,
					const LocalInfo &localInfo) {
			return	!keyInfo.limitations.scores
					|| *keyInfo.limitations.scores > localInfo.scores;
		}

		inline static bool CheckScores(
					const FeatureInfo &keyInfo,
					const LocalInfo &localInfo) {
			return	!keyInfo.limitations.scores
					|| *keyInfo.limitations.scores > localInfo.scores;
		}

		inline static bool IsLicenseKeyActive(
					const LicenseKeyInfo &keyInfo,
					const LocalInfo &localInfo) {
			return
				CheckTime(keyInfo, localInfo)
				&& CheckScores(keyInfo, localInfo)
				&& CheckWorkstationPropetiesChanges(keyInfo, localInfo);
		}

		inline static bool IsFeatureActive(
					const FeatureInfo &keyInfo,
					const LocalInfo &localInfo) {
			return CheckTime(keyInfo, localInfo)
				&& CheckScores(keyInfo, localInfo)
				&& CheckWorkstationPropetiesChanges(keyInfo, localInfo);
		}

		inline static bool IsFeatureAvailable(
					const FeatureValue &keyVal,
					const FeatureValue &localVal) {
			typedef ValueCheckPolicy<ClientTrait, FeatureValue> ValueCheck;
			return ValueCheck::IsAvailable(keyVal, localVal);
		}

	};

} }

#endif // #ifndef INCLUDED_FILE__TUNNELEX__CheckPolicies_hpp__0909081926
