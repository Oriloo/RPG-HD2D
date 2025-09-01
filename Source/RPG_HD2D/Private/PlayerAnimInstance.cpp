#include "PlayerAnimInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"

UPlayerAnimInstance::UPlayerAnimInstance()
{
	Speed = 0.0f;
	bIsMoving = false;
	Velocity = FVector::ZeroVector;
	OwnerPawn = nullptr;
}

void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	OwnerPawn = TryGetPawnOwner();
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (OwnerPawn)
	{
		if (UPawnMovementComponent* MovementComponent = OwnerPawn->GetMovementComponent())
		{
			Velocity = MovementComponent->Velocity;
			Speed = Velocity.Size();
			bIsMoving = Speed > 3.0f;
		}
	}
}