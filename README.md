## 해당 플러그인은 mklabs - Target System Component Plugin를 참고하여 제작 ([fabs](https://www.fab.com/ko/listings/9088334d-3bde-4e10-a937-baeb780f880f))  

### 개선 방식
1. 멀티플레이 사용을 위한 리플리케이션 방식 적용  
2. 오너(플레이어)와 가장 가까운 거리의 액터 -> 플레이어의 뷰포트 중심에서 가장 가까운 대상 선택  
3. ActorIterator()를 통해 월드 내 모든 액터 대상 -> 여러 Phase를 통해 일부 검사  

### 타겟 선별 방식
1. SweepPhase : 카메라가 보고 있는 방향으로 Capsule 검사(OverlapMulti)  
2. LinePhase : Sweep Multi를 통해 검출된 대상과 오너를 Line Trace Single을 통해 사이에 장애물이 있으면 제외  
3. FindNearestActor : 액터의 월드 좌표를 사영(Proj)시켜 2차원 좌표로 변경 후 카메라의 중점과 가장 가까운 대상 선별  

### 틱  
타겟---(장애물)---오너 : 취소  
타겟---(다른 액터)---오너 : 허용  
이를 위해서 Trace Multi로 구현, 타겟은 Visibility 무시, 플레이어는 Visibility 오버랩으로 해주어야 막히지 않고 투과함  

### 키보드 입력  
마우스 입력의 X축 값을 통해 옆 타겟으로 옮길 수 있어야 한다  
타이머를 통해 쿨타임을 구현, 타겟의 옆으로 Sphere 검사(OverlapMulti)  
타겟 액터와 가장 가까운 액터를 타겟으로 변경  
